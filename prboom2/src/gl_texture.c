/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_texture.c,v 1.13 2000/10/08 18:42:19 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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

#include "gl_intern.h"
#include "gl_struct.h"

extern int numtextures;
extern texture_t **textures;

/* TEXTURES */
static GLTexture **gld_GLTextures=NULL;
/* PATCHES FLATS SPRITES */
static GLTexture **gld_GLPatchTextures=NULL;

#ifdef USE_GLU_MIPMAP
boolean use_mipmapping=false;
#endif

int gld_max_texturesize=0;
char *gl_tex_format_string;
//int gl_tex_format=GL_RGBA8;
int gl_tex_format=GL_RGB5_A1;
//int gl_tex_format=GL_RGBA4;
//int gl_tex_format=GL_RGBA2;

GLTexture *last_gltexture=NULL;

int gld_GetTexDimension(int value)
{
  int i;

  i=1;
  while (i<value)
    i*=2;
  if (i>gld_max_texturesize)
    i=gld_max_texturesize;
  return i;
}

static GLTexture *gld_AddNewGLTexture(int texture_num)
{
  if (texture_num<0)
    return NULL;
  if (texture_num>=numtextures)
    return NULL;
  if (!gld_GLTextures)
  {
    gld_GLTextures=Z_Malloc(numtextures*sizeof(GLTexture *),PU_STATIC,0);
    memset(gld_GLTextures,0,numtextures*sizeof(GLTexture *));
  }
  if (!gld_GLTextures[texture_num])
  {
    gld_GLTextures[texture_num]=Z_Malloc(sizeof(GLTexture),PU_STATIC,0);
    memset(gld_GLTextures[texture_num], 0, sizeof(GLTexture));
    gld_GLTextures[texture_num]->textype=GLDT_UNREGISTERED;
  }
  return gld_GLTextures[texture_num];
}

static GLTexture *gld_AddNewGLPatchTexture(int lump)
{
  if (lump<0)
    return NULL;
  if (lump>=numlumps)
    return NULL;
  if (!gld_GLPatchTextures)
  {
    gld_GLPatchTextures=Z_Malloc(numlumps*sizeof(GLTexture *),PU_STATIC,0);
    memset(gld_GLPatchTextures,0,numlumps*sizeof(GLTexture *));
  }
  if (!gld_GLPatchTextures[lump])
  {
    gld_GLPatchTextures[lump]=Z_Malloc(sizeof(GLTexture),PU_STATIC,0);
    memset(gld_GLPatchTextures[lump], 0, sizeof(GLTexture));
    gld_GLPatchTextures[lump]->textype=GLDT_UNREGISTERED;
  }
  return gld_GLPatchTextures[lump];
}

static void gld_AddPatchToTexture_UnTranslated(GLTexture *gltexture, unsigned char *buffer, const patch_t *patch, int originx, int originy)
{
  int x,y,j;
  int xs,xe;
  int js,je;
  column_t *p_bColumn_t;
  byte *p_bColumn;
  int pos;
  const unsigned char *playpal;

  if (!gltexture)
    return;
  if (!patch)
    return;
  playpal=W_CacheLumpName("PLAYPAL");
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
  p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[0]);
	for (x=xs;x<xe;x++)
	{
#ifdef RANGECHECK
    if (x>=patch->width)
    {
      lprintf(LO_ERROR,"gld_AddPatchToTexture_UnTranslated x>=patch->width (%i >= %i)\n",x,patch->width);
      return;
    }
#endif
    p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[x]);
    while (p_bColumn_t->topdelta != 0xff)
  	{
      y=(p_bColumn_t->topdelta+originy);
      js=0;
      je=p_bColumn_t->length;
      if ((js+y)>=gltexture->realtexheight)
        goto nextrun;
      if ((je+y)<=0)
        goto nextrun;
      if ((js+y)<0)
        js=-y;
      if ((je+y)>gltexture->realtexheight)
        je+=(gltexture->realtexheight-(je+y));
			p_bColumn=(byte *)p_bColumn_t + 3;
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
				buffer[pos]=playpal[p_bColumn[j]*3];
				buffer[pos+1]=playpal[p_bColumn[j]*3+1];
				buffer[pos+2]=playpal[p_bColumn[j]*3+2];
        buffer[pos+3]=255;
			}
nextrun:
			p_bColumn_t = (column_t *)(  (byte *)p_bColumn_t + p_bColumn_t->length + 4);
		}
	}
  W_UnlockLumpName("PLAYPAL");
}

void gld_AddPatchToTexture(GLTexture *gltexture, unsigned char *buffer, const patch_t *patch, int originx, int originy, int cm)
{
  int x,y,j;
  int xs,xe;
  int js,je;
  column_t *p_bColumn_t;
  byte *p_bColumn;
  int pos;
  const unsigned char *playpal;
  extern const unsigned char *colrngs[];
  const unsigned char *outr;

  if (!gltexture)
    return;
  if (!patch)
    return;
  if ((cm==CR_DEFAULT) || (cm==CR_LIMIT))
  {
    gld_AddPatchToTexture_UnTranslated(gltexture,buffer,patch,originx,originy);
    return;
  }
  if (cm<CR_LIMIT)
    outr=colrngs[cm];
  else
    outr=translationtables + 256*((cm-CR_LIMIT)-1);
  playpal=W_CacheLumpName("PLAYPAL");
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
  p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[0]);
	for (x=xs;x<xe;x++)
	{
#ifdef RANGECHECK
    if (x>=patch->width)
    {
      lprintf(LO_ERROR,"gld_AddPatchToTexture x>=patch->width (%i >= %i)\n",x,patch->width);
      return;
    }
#endif
    p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[x]);
    while (p_bColumn_t->topdelta != 0xff)
  	{
      y=(p_bColumn_t->topdelta+originy);
      js=0;
      je=p_bColumn_t->length;
      if ((js+y)>=gltexture->realtexheight)
        goto nextrun;
      if ((je+y)<=0)
        goto nextrun;
      if ((js+y)<0)
        js=-y;
      if ((je+y)>gltexture->realtexheight)
        je+=(gltexture->realtexheight-(je+y));
			p_bColumn=(byte *)p_bColumn_t + 3;
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
				buffer[pos]=playpal[outr[p_bColumn[j]]*3];
				buffer[pos+1]=playpal[outr[p_bColumn[j]]*3+1];
				buffer[pos+2]=playpal[outr[p_bColumn[j]]*3+2];
        buffer[pos+3]=255;
			}
nextrun:
			p_bColumn_t = (column_t *)(  (byte *)p_bColumn_t + p_bColumn_t->length + 4);
		}
	}
  W_UnlockLumpName("PLAYPAL");
}

static void gld_AddFlatToTexture(GLTexture *gltexture, unsigned char *buffer, const unsigned char *flat)
{
  int x,y,pos;
  const unsigned char *playpal;

  if (!gltexture)
    return;
  if (!flat)
    return;
  playpal=W_CacheLumpName("PLAYPAL");
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
  		buffer[pos]=playpal[flat[y*64+x]*3];
			buffer[pos+1]=playpal[flat[y*64+x]*3+1];
	  	buffer[pos+2]=playpal[flat[y*64+x]*3+2];
			buffer[pos+3]=255;
    }
  }
  W_UnlockLumpName("PLAYPAL");
}

GLTexture *gld_RegisterTexture(int texture_num, boolean mipmap)
{
  GLTexture *gltexture;

  if (texture_num==R_TextureNumForName("-"))
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
    gltexture->mipmap=mipmap;
    gltexture->realtexwidth=texture->width;
    gltexture->realtexheight=texture->height;
    gltexture->leftoffset=0;
    gltexture->topoffset=0;
    gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
    gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
    gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=gltexture->tex_width;
    gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
    gltexture->width=gltexture->tex_width;
    gltexture->height=gltexture->tex_height;
    gltexture->buffer_width=gltexture->realtexwidth;
    gltexture->buffer_height=gltexture->realtexheight;
#endif
#ifdef USE_GLU_MIPMAP
    if (gltexture->mipmap & use_mipmapping)
    {
      gltexture->width=gltexture->tex_width;
      gltexture->height=gltexture->tex_height;
      gltexture->buffer_width=gltexture->realtexwidth;
      gltexture->buffer_height=gltexture->realtexheight;
    }
#endif
    gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
    if (gltexture->realtexwidth>gltexture->buffer_width)
      return gltexture;
    if (gltexture->realtexheight>gltexture->buffer_height)
      return gltexture;
    gltexture->textype=GLDT_TEXTURE;
  }
  return gltexture;
}

void gld_BindTexture(GLTexture *gltexture)
{
  const patch_t *patch;
  int i;
  unsigned char *buffer;
  texture_t *texture;

  if (gltexture==last_gltexture)
    return;
  last_gltexture=gltexture;
  if (!gltexture)
    return;
  if (gltexture->textype!=GLDT_TEXTURE)
  {
    glDisable(GL_TEXTURE_2D);
    return;
  }
  else
    glEnable(GL_TEXTURE_2D);
  if (gltexture->glTexID[CR_DEFAULT]!=0)
  {
   	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_RESIDENT,&i);
#ifdef _DEBUG
    if (i!=GL_TRUE)
      lprintf(LO_INFO, "glGetTexParam: %i\n", i);
#endif
    if (i==GL_TRUE)
      return;
  }
  buffer=(unsigned char*)Z_Malloc(gltexture->buffer_size,PU_STATIC,0);
  memset(buffer,0,gltexture->buffer_size);
  texture=textures[gltexture->index];
  for (i=0; i<texture->patchcount; i++)
  {
    patch=W_CacheLumpNum(texture->patches[i].patch);
    gld_AddPatchToTexture(gltexture, buffer, patch,
                          texture->patches[i].originx,
                          texture->patches[i].originy,
                          CR_DEFAULT);
    W_UnlockLumpNum(texture->patches[i].patch);
  }
  if (gltexture->glTexID[CR_DEFAULT]==0)
    glGenTextures(1,&gltexture->glTexID[CR_DEFAULT]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
#ifdef USE_GLU_MIPMAP
  if (gltexture->mipmap & use_mipmapping)
  {
	  gluBuild2DMipmaps(GL_TEXTURE_2D, gl_tex_format,
					            gltexture->buffer_width, gltexture->buffer_height,
					            GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_mipmap_filter);
  }
  else
#endif /* USE_GLU_MIPMAP */
  {
#ifdef USE_GLU_IMAGESCALE
    if ((gltexture->buffer_width!=gltexture->tex_width) ||
        (gltexture->buffer_height!=gltexture->tex_height)
       )
    {
      unsigned char *scaledbuffer;

      scaledbuffer=(unsigned char*)Z_Malloc(gltexture->tex_width*gltexture->tex_height*4,PU_STATIC,0);
      if (scaledbuffer)
      {
        gluScaleImage(GL_RGBA,
                      gltexture->buffer_width, gltexture->buffer_height,
                      GL_UNSIGNED_BYTE,buffer,
                      gltexture->tex_width, gltexture->tex_height,
                      GL_UNSIGNED_BYTE,scaledbuffer);
        Z_Free(buffer);
        buffer=scaledbuffer;
        glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                      gltexture->tex_width, gltexture->tex_height,
                      0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
      }
    }
    else
#endif /* USE_GLU_IMAGESCALE */
    {
      glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                    gltexture->buffer_width, gltexture->buffer_height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);
  }
  Z_Free(buffer);
}

GLTexture *gld_RegisterPatch(int lump, int cm)
{
  const patch_t *patch;
  GLTexture *gltexture;

  gltexture=gld_AddNewGLPatchTexture(lump);
  if (!gltexture)
    return NULL;
  if (gltexture->textype==GLDT_UNREGISTERED)
  {
    patch=W_CacheLumpNum(lump);
    if (!patch)
      return NULL;
    gltexture->textype=GLDT_BROKEN;
    gltexture->index=lump;
    gltexture->mipmap=false;
    gltexture->realtexwidth=patch->width;
    gltexture->realtexheight=patch->height;
    gltexture->leftoffset=patch->leftoffset;
    gltexture->topoffset=patch->topoffset;
    gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
    gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
    gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=gltexture->tex_width;
    gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
    gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=max(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->buffer_height=max(gltexture->realtexheight, gltexture->tex_height);
#endif
    gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
    W_UnlockLumpNum(lump);
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
  const patch_t *patch;
  int i;
  unsigned char *buffer;

  if (gltexture==last_gltexture)
    return;
  last_gltexture=gltexture;
  if (!gltexture)
    return;
  if (gltexture->textype!=GLDT_PATCH)
  {
    glDisable(GL_TEXTURE_2D);
    return;
  }
  else
    glEnable(GL_TEXTURE_2D);
  if (gltexture->glTexID[cm]!=0)
  {
 	  glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_RESIDENT,&i);
#ifdef _DEBUG
    if (i!=GL_TRUE)
      lprintf(LO_INFO, "glGetTexParam: %i\n", i);
#endif
    if (i==GL_TRUE)
      return;
  }
  patch=W_CacheLumpNum(gltexture->index);
  buffer=(unsigned char*)Z_Malloc(gltexture->buffer_size,PU_STATIC,0);
  memset(buffer,0,gltexture->buffer_size);
  gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0, cm);
  if (gltexture->glTexID[cm]==0)
	  glGenTextures(1,&gltexture->glTexID[cm]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
#ifdef USE_GLU_IMAGESCALE
  if ((gltexture->buffer_width>gltexture->tex_width) ||
      (gltexture->buffer_height>gltexture->tex_height)
     )
  {
    unsigned char *scaledbuffer;

    scaledbuffer=(unsigned char*)Z_Malloc(gltexture->tex_width*gltexture->tex_height*4,PU_STATIC,0);
    if (scaledbuffer)
    {
      gluScaleImage(GL_RGBA,
                    gltexture->buffer_width, gltexture->buffer_height,
                    GL_UNSIGNED_BYTE,buffer,
                    gltexture->tex_width, gltexture->tex_height,
                    GL_UNSIGNED_BYTE,scaledbuffer);
      Z_Free(buffer);
      buffer=scaledbuffer;
      glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                    gltexture->tex_width, gltexture->tex_height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
  }
  else
#endif /* USE_GLU_IMAGESCALE */
  {
    glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format, 
                  gltexture->buffer_width, gltexture->buffer_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  }
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);
  Z_Free(buffer);
  W_UnlockLumpNum(gltexture->index);
}

GLTexture *gld_RegisterFlat(int lump, boolean mipmap)
{
  GLTexture *gltexture;

  gltexture=gld_AddNewGLPatchTexture(firstflat+lump);
  if (!gltexture)
    return NULL;
  if (gltexture->textype==GLDT_UNREGISTERED)
  {
    gltexture->textype=GLDT_BROKEN;
    gltexture->index=firstflat+lump;
    gltexture->mipmap=mipmap;
    gltexture->realtexwidth=64;
    gltexture->realtexheight=64;
    gltexture->leftoffset=0;
    gltexture->topoffset=0;
    gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
    gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
    gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
    gltexture->buffer_width=gltexture->tex_width;
    gltexture->buffer_height=gltexture->tex_height;
#ifdef USE_GLU_IMAGESCALE
    gltexture->width=gltexture->tex_width;
    gltexture->height=gltexture->tex_height;
    gltexture->buffer_width=gltexture->realtexwidth;
    gltexture->buffer_height=gltexture->realtexheight;
#endif
#ifdef USE_GLU_MIPMAP
    if (gltexture->mipmap & use_mipmapping)
    {
      gltexture->width=gltexture->tex_width;
      gltexture->height=gltexture->tex_height;
      gltexture->buffer_width=gltexture->realtexwidth;
      gltexture->buffer_height=gltexture->realtexheight;
    }
#endif
    gltexture->buffer_size=gltexture->buffer_width*gltexture->buffer_height*4;
    if (gltexture->realtexwidth>gltexture->buffer_width)
      return gltexture;
    if (gltexture->realtexheight>gltexture->buffer_height)
      return gltexture;
    gltexture->textype=GLDT_FLAT;
  }
  return gltexture;
}

void gld_BindFlat(GLTexture *gltexture)
{
  const unsigned char *flat;
  int i;
  unsigned char *buffer;

  if (gltexture==last_gltexture)
    return;
  last_gltexture=gltexture;
  if (!gltexture)
    return;
  if (gltexture->textype!=GLDT_FLAT)
  {
    glDisable(GL_TEXTURE_2D);
    return;
  }
  else
    glEnable(GL_TEXTURE_2D);
  if (gltexture->glTexID[CR_DEFAULT]!=0)
  {
   	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_RESIDENT,&i);
#ifdef _DEBUG
    if (i!=GL_TRUE)
      lprintf(LO_INFO, "glGetTexParam: %i\n", i);
#endif
    if (i==GL_TRUE)
      return;
  }
  flat=W_CacheLumpNum(gltexture->index);
  buffer=(unsigned char*)Z_Malloc(gltexture->buffer_size,PU_STATIC,0);
  memset(buffer,0,gltexture->buffer_size);
  gld_AddFlatToTexture(gltexture, buffer, flat);
  if (gltexture->glTexID[CR_DEFAULT]==0)
  	glGenTextures(1,&gltexture->glTexID[CR_DEFAULT]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
#ifdef USE_GLU_MIPMAP
  if (gltexture->mipmap & use_mipmapping)
  {
	  gluBuild2DMipmaps(GL_TEXTURE_2D, gl_tex_format,
                      gltexture->buffer_width, gltexture->buffer_height,
					            GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_mipmap_filter);
  }
  else
#endif /* USE_GLU_MIPMAP */
  {
#ifdef USE_GLU_IMAGESCALE
    if ((gltexture->buffer_width!=gltexture->tex_width) ||
        (gltexture->buffer_height!=gltexture->tex_height)
       )
    {
      unsigned char *scaledbuffer;

      scaledbuffer=(unsigned char*)Z_Malloc(gltexture->tex_width*gltexture->tex_height*4,PU_STATIC,0);
      if (scaledbuffer)
      {
        gluScaleImage(GL_RGBA,
                      gltexture->buffer_width, gltexture->buffer_height,
                      GL_UNSIGNED_BYTE,buffer,
                      gltexture->tex_width, gltexture->tex_height,
                      GL_UNSIGNED_BYTE,scaledbuffer);
        Z_Free(buffer);
        buffer=scaledbuffer;
        glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                      gltexture->tex_width, gltexture->tex_height,
                      0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
      }
    }
    else
#endif /* USE_GLU_IMAGESCALE */
    {
      glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format, 
                    gltexture->buffer_width, gltexture->buffer_height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);
  }
  Z_Free(buffer);
  W_UnlockLumpNum(gltexture->index);
}

static void gld_CleanTextures(void)
{
  int i,j;

  if (!gld_GLTextures)
    return;
  for (i=0; i<numtextures; i++)
  {
    if (gld_GLTextures[i])
    {
      for (j=0; j<(CR_LIMIT+MAXPLAYERS); j++)
        glDeleteTextures(1,&(gld_GLTextures[i]->glTexID[j]));
      Z_Free(gld_GLTextures[i]);
    }
  }
  memset(gld_GLTextures,0,numtextures*sizeof(GLTexture *));
}

static void gld_CleanPatchTextures(void)
{
  int i,j;

  if (!gld_GLPatchTextures)
    return;
  for (i=0; i<numlumps; i++)
  {
    if (gld_GLPatchTextures[i])
    {
      for (j=0; j<(CR_LIMIT+MAXPLAYERS); j++)
        glDeleteTextures(1,&(gld_GLPatchTextures[i]->glTexID[j]));
      Z_Free(gld_GLPatchTextures[i]);
    }
  }
  memset(gld_GLPatchTextures,0,numlumps*sizeof(GLTexture *));
}

void gld_Precache(void)
{
  extern int firstflat, lastflat, numflats;
  extern int firstspritelump, lastspritelump, numspritelumps;
  extern int numtextures;
  register int i;
  register byte *hitlist;

  if (demoplayback)
    return;

  {
    size_t size = numflats > numsprites  ? numflats : numsprites;
    hitlist = Z_Malloc((size_t)numtextures > size ? numtextures : size,PU_LEVEL,0);
  }

  // Precache flats.

  memset(hitlist, 0, numflats);

  for (i = numsectors; --i >= 0; )
    hitlist[sectors[i].floorpic] = hitlist[sectors[i].ceilingpic] = 1;

  for (i = numflats; --i >= 0; )
    if (hitlist[i])
      gld_BindFlat(gld_RegisterFlat(i,true));

  // Precache textures.

  memset(hitlist, 0, numtextures);

  for (i = numsides; --i >= 0;)
    hitlist[sides[i].bottomtexture] =
      hitlist[sides[i].toptexture] =
      hitlist[sides[i].midtexture] = 1;

  // Sky texture is always present.
  // Note that F_SKY1 is the name used to
  //  indicate a sky floor/ceiling as a flat,
  //  while the sky texture is stored like
  //  a wall texture, with an episode dependend
  //  name.

  hitlist[skytexture] = 0;

  for (i = numtextures; --i >= 0; )
    if (hitlist[i])
      gld_BindTexture(gld_RegisterTexture(i,true));

  // Precache sprites.
  memset(hitlist, 0, numsprites);

  {
    thinker_t *th;
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
      if (th->function == P_MobjThinker)
        hitlist[((mobj_t *)th)->sprite] = 1;
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
              gld_BindPatch(gld_RegisterPatch(firstspritelump + sflump[k],CR_DEFAULT),CR_DEFAULT);
            while (--k >= 0);
          }
      }
  Z_Free(hitlist);
}

void gld_CleanMemory(void)
{
  gld_CleanTextures();
  gld_CleanPatchTextures();
}
