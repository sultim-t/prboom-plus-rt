/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: gl_texture.c,v 1.7 2000/05/13 18:39:55 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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

boolean use_mipmaping;
int gld_max_texturesize=0;

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
    gld_GLTextures=GLMalloc(numtextures*sizeof(GLTexture *));
    memset(gld_GLTextures,0,numtextures*sizeof(GLTexture *));
  }
  if (!gld_GLTextures[texture_num])
  {
    gld_GLTextures[texture_num]=GLMalloc(sizeof(GLTexture));
    memset(gld_GLTextures[texture_num], 0, sizeof(GLTexture));
    gld_GLTextures[texture_num]->registered=false;
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
    gld_GLPatchTextures=GLMalloc(numlumps*sizeof(GLTexture *));
    memset(gld_GLPatchTextures,0,numlumps*sizeof(GLTexture *));
  }
  if (!gld_GLPatchTextures[lump])
  {
    gld_GLPatchTextures[lump]=GLMalloc(sizeof(GLTexture));
    memset(gld_GLPatchTextures[lump], 0, sizeof(GLTexture));
    gld_GLPatchTextures[lump]->registered=false;
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
  int size;
  const unsigned char *playpal;

  if (!gltexture)
    return;
  if (!patch)
    return;
  if (gltexture->width>gltexture->tex_width)
    return;
  if (gltexture->height>gltexture->tex_height)
    return;
  playpal=W_CacheLumpName("PLAYPAL");
  size=gltexture->size;
  xs=0;
  xe=patch->width;
  if ((xs+originx)>=gltexture->width)
    return;
  if ((xe+originx)<=0)
    return;
  if ((xs+originx)<0)
    xs=-originx;
  if ((xe+originx)>gltexture->width)
    xe+=(gltexture->width-(xe+originx));
  p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[0]);
	for (x=xs;x<xe;x++)
	{
#ifdef RANGECHECK
    if (x>=patch->width)
      I_Error("gld_AddPatchToTexture_UnTranslated x>=patch->width (%i >= %i)",x,patch->width);
#endif
    p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[x]);
    while (p_bColumn_t->topdelta != 0xff)
  	{
      y=(p_bColumn_t->topdelta+originy);
      js=0;
      je=p_bColumn_t->length;
      if ((js+y)>=gltexture->height)
        goto nextrun;
      if ((je+y)<=0)
        goto nextrun;
      if ((js+y)<0)
        js=-y;
      if ((je+y)>gltexture->height)
        je+=(gltexture->height-(je+y));
			p_bColumn=(byte *)p_bColumn_t + 3;
      pos=4*(((js+y)*gltexture->tex_width)+x+originx);
			for (j=js;j<je;j++,pos+=(4*gltexture->tex_width))
			{	
#ifdef RANGECHECK
        if ((pos+3)>=size)
          I_Error("gld_AddPatchToTexture_UnTranslated pos+3>=size (%i >= %i)",pos+3,size);
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
  int size;
  const unsigned char *playpal;
  extern const unsigned char *colrngs[];
  const unsigned char *outr;

  if (!gltexture)
    return;
  if (!patch)
    return;
  if (gltexture->width>gltexture->tex_width)
    return;
  if (gltexture->height>gltexture->tex_height)
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
  size=gltexture->size;
  xs=0;
  xe=patch->width;
  if ((xs+originx)>=gltexture->width)
    return;
  if ((xe+originx)<=0)
    return;
  if ((xs+originx)<0)
    xs=-originx;
  if ((xe+originx)>gltexture->width)
    xe+=(gltexture->width-(xe+originx));
  p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[0]);
	for (x=xs;x<xe;x++)
	{
#ifdef RANGECHECK
    if (x>=patch->width)
      I_Error("gld_AddPatchToTexture x>=patch->width (%i >= %i)",x,patch->width);
#endif
    p_bColumn_t=(column_t *)((byte *)patch+patch->columnofs[x]);
    while (p_bColumn_t->topdelta != 0xff)
  	{
      y=(p_bColumn_t->topdelta+originy);
      js=0;
      je=p_bColumn_t->length;
      if ((js+y)>=gltexture->height)
        goto nextrun;
      if ((je+y)<=0)
        goto nextrun;
      if ((js+y)<0)
        js=-y;
      if ((je+y)>gltexture->height)
        je+=(gltexture->height-(je+y));
			p_bColumn=(byte *)p_bColumn_t + 3;
      pos=4*(((js+y)*gltexture->tex_width)+x+originx);
			for (j=js;j<je;j++,pos+=(4*gltexture->tex_width))
			{	
#ifdef RANGECHECK
        if ((pos+3)>=size)
          I_Error("gld_AddPatchToTexture pos+3>=size (%i >= %i)",pos+3,size);
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
  if (gltexture->width>gltexture->tex_width)
    return;
  if (gltexture->height>gltexture->tex_height)
    return;
  playpal=W_CacheLumpName("PLAYPAL");
  for (y=0;y<gltexture->height;y++)
  {
    pos=4*(y*gltexture->tex_width);
    for (x=0;x<gltexture->width;x++,pos+=4)
    {
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
  if (!gltexture->registered)
  {
    if (!textures[texture_num])
      return NULL;
    gltexture->registered=true;
    gltexture->texture=textures[texture_num];
    gltexture->mipmap=mipmap;
    gltexture->width=gltexture->texture->width;
    gltexture->height=gltexture->texture->height;
    gltexture->leftoffset=0;
    gltexture->topoffset=0;
    gltexture->tex_width=gld_GetTexDimension(gltexture->width);
    gltexture->tex_height=gld_GetTexDimension(gltexture->height);
    gltexture->size=gltexture->tex_width*gltexture->tex_height*4;
  }
  return gltexture;
}

void gld_BindTexture(GLTexture *gltexture)
{
  const patch_t *patch;
  int i;
  unsigned char *buffer;

  if (!gltexture)
    return;
  if (!gltexture->registered)
    return;
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
  buffer=(unsigned char*)GLMalloc(gltexture->size);
  memset(buffer,0,gltexture->size);
  for (i=0; i<gltexture->texture->patchcount; i++)
  {
    patch=W_CacheLumpNum(gltexture->texture->patches[i].patch);
/*
    if (gltexture->texture==textures[skytexture])
      gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0, CR_DEFAULT);
    else
*/
    gld_AddPatchToTexture(gltexture, buffer, patch,
                          gltexture->texture->patches[i].originx,
                          gltexture->texture->patches[i].originy,
                          CR_DEFAULT);
    W_UnlockLumpNum(gltexture->texture->patches[i].patch);
  }
  if (gltexture->glTexID[CR_DEFAULT]==0)
    glGenTextures(1,&gltexture->glTexID[CR_DEFAULT]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
  if (gltexture->mipmap & use_mipmaping)
  {
	 gluBuild2DMipmaps( GL_TEXTURE_2D, 4,
					            gltexture->tex_width, gltexture->tex_height,
					            GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }
  else
  {
    /*
    if ((gltexture->width!=gltexture->tex_width) || (gltexture->height!=gltexture->tex_height))
    {
      unsigned char *scaledbuffer;

      scaledbuffer=(unsigned char*)GLMalloc(gltexture->size);
      if (scaledbuffer)
      {
        gluScaleImage(GL_RGBA
        GLFree(buffer);
        buffer=scaledbuffer;
      }
    }
    */
    glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                  gltexture->tex_width, gltexture->tex_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
  GLFree(buffer);
}

GLTexture *gld_RegisterPatch(int lump, int cm)
{
  const patch_t *patch;
  GLTexture *gltexture;

  gltexture=gld_AddNewGLPatchTexture(lump);
  if (!gltexture)
    return NULL;
  if (!gltexture->registered)
  {
    patch=W_CacheLumpNum(lump);
    if (!patch)
      return NULL;
    gltexture->registered=true;
    gltexture->lump=lump;
    gltexture->width=patch->width;
    gltexture->height=patch->height;
    gltexture->leftoffset=patch->leftoffset;
    gltexture->topoffset=patch->topoffset;
    gltexture->tex_width=gld_GetTexDimension(gltexture->width);
    gltexture->tex_height=gld_GetTexDimension(gltexture->height);
    gltexture->size=gltexture->tex_width*gltexture->tex_height*4;
    W_UnlockLumpNum(lump);
  }
  return gltexture;
}

void gld_BindPatch(GLTexture *gltexture, int cm)
{
  const patch_t *patch;
  int i;
  unsigned char *buffer;

  if (!gltexture)
    return;
  if (!gltexture->registered)
    return;
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
  patch=W_CacheLumpNum(gltexture->lump);
  buffer=(unsigned char*)GLMalloc(gltexture->size);
  memset(buffer,0,gltexture->size);
  gld_AddPatchToTexture(gltexture, buffer, patch, 0, 0, cm);
  if (gltexture->glTexID[cm]==0)
	  glGenTextures(1,&gltexture->glTexID[cm]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
  glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                gltexture->tex_width, gltexture->tex_height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GLFree(buffer);
  W_UnlockLumpNum(gltexture->lump);
}

GLTexture *gld_RegisterFlat(int lump, boolean mipmap)
{
  GLTexture *gltexture;

  gltexture=gld_AddNewGLPatchTexture(lump);
  if (!gltexture)
    return NULL;
  if (!gltexture->registered)
  {
    gltexture->registered=true;
    gltexture->lump=lump;
    gltexture->width=64;
    gltexture->height=64;
    gltexture->leftoffset=0;
    gltexture->topoffset=0;
    gltexture->tex_width=gld_GetTexDimension(gltexture->width);
    gltexture->tex_height=gld_GetTexDimension(gltexture->height);
    gltexture->size=gltexture->tex_width*gltexture->tex_height*4;
  }
  return gltexture;
}

void gld_BindFlat(GLTexture *gltexture)
{
  const unsigned char *flat;
  int i;
  unsigned char *buffer;

  if (!gltexture)
    return;
  if (!gltexture->registered)
    return;
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
  flat=W_CacheLumpNum(gltexture->lump);
  buffer=(unsigned char*)GLMalloc(gltexture->size);
  memset(buffer,0,gltexture->size);
  gld_AddFlatToTexture(gltexture, buffer, flat);
  if (gltexture->glTexID[CR_DEFAULT]==0)
  	glGenTextures(1,&gltexture->glTexID[CR_DEFAULT]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
  if (gltexture->mipmap & use_mipmaping)
  {
	 gluBuild2DMipmaps( GL_TEXTURE_2D, 4,
					            gltexture->tex_width, gltexture->tex_height,
					            GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  }
  else
  {
    glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                  gltexture->tex_width, gltexture->tex_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
  GLFree(buffer);
  W_UnlockLumpNum(gltexture->lump);
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
      for (j=0; j<CR_LIMIT; j++)
        glDeleteTextures(1,&(gld_GLTextures[i]->glTexID[j]));
      GLFree(gld_GLTextures[i]);
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
      for (j=0; j<CR_LIMIT; j++)
        glDeleteTextures(1,&(gld_GLPatchTextures[i]->glTexID[j]));
      GLFree(gld_GLPatchTextures[i]);
    }
  }
  memset(gld_GLPatchTextures,0,numlumps*sizeof(GLTexture *));
}

#ifdef _DEBUG
void gld_Precache(void)
{
  int i;
  GLTexture* gltexture;

  for (i=0;i<numtextures;i++)
  {
    gltexture=gld_RegisterTexture(i,false);
    gld_BindTexture(gltexture);
  }
}
#endif

void gld_CleanMemory(void)
{
  gld_CleanTextures();
  gld_CleanPatchTextures();
}
