// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: gl_texture.c,v 1.1 2000/05/04 16:40:00 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//
//---------------------------------------------------------------------

#include "gl_intern.h"
#include "gl_struct.h"
#include "lprintf.h"

static int *gld_TexNumToGLTexture=NULL;
static int gld_NumTex=0;
static GLTexture *gld_GLTextures=NULL;
static int gld_NumGLTextures=0;

extern texture_t **textures;
extern int gld_GetTexHeightGL(int value);
extern void gld_AddPatchToTexture(GLTexture *gltexture, unsigned char *buffer, const patch_t *patch, int originx, int originy);

static GLTexture *gld_AddNewGLTexture(int texture_num)
{
  int i;

  if (texture_num<0)
    return NULL;
  if (texture_num>(gld_NumTex-1))
  {
    gld_TexNumToGLTexture=(int *)GLRealloc(gld_TexNumToGLTexture, (texture_num+1)*sizeof(int));
    if (gld_TexNumToGLTexture==NULL)
    {
      lprintf(LO_INFO, "gld_TexNumToGLTexture==NULL\n");
      return NULL;
    }
    for (i=gld_NumTex; i<=texture_num; i++)
      gld_TexNumToGLTexture[i]=-1;
    gld_NumTex=texture_num+1;
  }
  if (gld_TexNumToGLTexture[texture_num]==-1)
  {
    gld_GLTextures=(GLTexture *)GLRealloc(gld_GLTextures, (gld_NumGLTextures+1)*sizeof(GLTexture));
    if (gld_GLTextures==NULL)
    {
      lprintf(LO_INFO, "gld_GLTextures==NULL\n");
      return NULL;
    }
    gld_TexNumToGLTexture[texture_num]=gld_NumGLTextures;
    memset(&gld_GLTextures[gld_TexNumToGLTexture[texture_num]], 0, sizeof(GLTexture));
    gld_NumGLTextures++;
  }
  return &gld_GLTextures[gld_TexNumToGLTexture[texture_num]];
}

GLTexture *gld_RegisterTexture(int texture_num)
{
  GLTexture *gltexture;
  texture_t *texture;
  const patch_t *patch;
  int size;
  int i;
  unsigned char *buffer;

  if (texture_num==R_TextureNumForName("-"))
    return NULL;
  gltexture=gld_AddNewGLTexture(texture_num);
  if (!gltexture)
    return NULL;
  if (gltexture->glTexID[CR_DEFAULT]!=0)
  {
   	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_RESIDENT,&i);
#ifdef _DEBUG
    if (i!=GL_TRUE)
      lprintf(LO_INFO, "glGetTexParam: %i\n", i);
#endif
    if (i==GL_TRUE)
      return gltexture;
  }
  texture=textures[texture_num];
  gltexture->width=texture->width;
  gltexture->height=texture->height;
  gltexture->leftoffset=0;
  gltexture->topoffset=0;
  gltexture->tex_width=gld_GetTexHeightGL(gltexture->width);
  gltexture->tex_height=gld_GetTexHeightGL(gltexture->height);
  size=gltexture->tex_width*gltexture->tex_height*4;
  buffer=(unsigned char*)GLMalloc(size);
  memset(buffer,0,size);
  for (i=0; i<texture->patchcount; i++)
  {
    patch=W_CacheLumpNum(texture->patches[i].patch);
    gld_AddPatchToTexture(gltexture, buffer, patch, texture->patches[i].originx, texture->patches[i].originy);
    W_UnlockLumpNum(texture->patches[i].patch);
  }
  if (gltexture->glTexID[CR_DEFAULT]==0)
    glGenTextures(1,&gltexture->glTexID[CR_DEFAULT]);
	glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
  glTexImage2D( GL_TEXTURE_2D, 0, 4, 
                gltexture->tex_width, gltexture->tex_height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GLFree(buffer);
  return gltexture;
}

GLTexture *gld_GetGLTexture( short sTexture,short xOffset,short yOffset,float *fU1,float *fU2,
					   float *fV1,float *fV2,float *fU1Off,float *fV2Off)
{
  GLTexture *gltexture;

  gltexture=gld_RegisterTexture(sTexture);
  if (!gltexture)
    return NULL;

	if (xOffset>=gltexture->width)
		xOffset=0;
	if (yOffset>=gltexture->height)
		yOffset=0;
			
	*fU1 = 0.0f;
	*fU2 = (float)gltexture->width/(float)gltexture->tex_width;
	*fU1Off= (float)(xOffset)/(float)gltexture->tex_width;

	*fV1 = 0.0f;
	*fV2 = (float)gltexture->height/(float)gltexture->tex_height;
	*fV2Off = (float)(yOffset)/(float)gltexture->tex_height;
	return gltexture;
}

void gld_CleanTextures(void)
{
  int i,j;

  for (i=0; i<gld_NumGLTextures; i++)
  {
    for (j=0; j<CR_LIMIT; j++)
      glDeleteTextures(1,&gld_GLTextures[i].glTexID[j]);
    memset(&gld_GLTextures[i],0,sizeof(GLTexture));
  }
  GLFree(gld_GLTextures);
  gld_GLTextures=NULL;
  gld_NumGLTextures=0;
  GLFree(gld_TexNumToGLTexture);
  gld_TexNumToGLTexture=NULL;
  gld_NumTex=0;
}

//-----------------------------------------------------------------------------
//
// $Log: gl_texture.c,v $
// Revision 1.1  2000/05/04 16:40:00  proff_fs
// added OpenGL stuff. Not complete yet.
// Only the playerview is rendered.
// The normal output is displayed in a small window.
// The level is only drawn in debugmode to the window.
//
// Revision 1.2  2000/04/10 21:13:13  proff_fs
// added Log to OpenGL files
//
// Revision 1.1.1.1  2000/04/09 18:21:44  proff_fs
// Initial login
//
//-----------------------------------------------------------------------------
