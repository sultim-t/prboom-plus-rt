/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
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

#include "config.h"
#include "z_zone.h"
#include "gl_intern.h"
#include "gl_struct.h"

/* TEXTURES */
static GLTexture **gld_GLTextures=NULL;
/* PATCHES FLATS SPRITES */
static GLTexture **gld_GLPatchTextures=NULL;

boolean use_mipmapping=false;

int gld_max_texturesize=0;
int gl_tex_format=GL_RGB5_A1;

GLTexture *last_gltexture=NULL;
int last_cm=-1;

int transparent_pal_index;
static unsigned char gld_palmap[256];

void gld_InitPalettedTextures(void)
{
  const unsigned char *playpal;
  int pal[256];
  int i,j;

  playpal=W_CacheLumpName("PLAYPAL");
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
  W_UnlockLumpName("PLAYPAL");
}
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

void gld_SetTexturePalette(GLenum target)
{
  const unsigned char *playpal;
  unsigned char pal[1024];
  int i;

  playpal=W_CacheLumpName("PLAYPAL");
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
  p_glColorTableEXT(target, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, pal);
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
    gltexture->textype=GLDT_TEXTURE;
  }
  return gltexture;
}

void gld_BindTexture(GLTexture *gltexture)
{
  unsigned char *buffer;

  if (gltexture==last_gltexture)
    return;
  last_gltexture=gltexture;
  if (!gltexture) {
    p_glBindTexture(GL_TEXTURE_2D, 0);
    last_gltexture = NULL;
    last_cm = -1;
    return;
  }
  if (gltexture->textype!=GLDT_TEXTURE)
  {
    p_glBindTexture(GL_TEXTURE_2D, 0);
    last_gltexture = NULL;
    last_cm = -1;
    return;
  }
  if (gltexture->glTexID[CR_DEFAULT]!=0)
  {
    int i;
   	p_glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
    p_glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_RESIDENT,&i);
#ifdef _DEBUG
    if (i!=GL_TRUE)
      lprintf(LO_INFO, "p_glGetTexParam: %i\n", i);
#endif
    if (i==GL_TRUE)
      return;
  }
  if (gltexture->glTexID[CR_DEFAULT]==0)
    p_glGenTextures(1,&gltexture->glTexID[CR_DEFAULT]);
	p_glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
  buffer = V_GetPlottedTexture32(gltexture->index, gltexture->width, gltexture->height, gltexture->tex_width, gltexture->tex_height, RDRAW_FILTER_POINT, RDRAW_MASKEDCOLUMNEDGE_SLOPED, true);
  p_glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                  gltexture->tex_width, gltexture->tex_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);
  //p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_mipmap_filter);
  free(buffer);
}

GLTexture *gld_RegisterPatch(int lump, int cm)
{
  const TPatch *patch;
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
    gltexture->mipmap=false;
    gltexture->realtexwidth=patch->width;
    gltexture->realtexheight=patch->height;
    gltexture->leftoffset=patch->leftOffset;
    gltexture->topoffset=patch->topOffset;
    gltexture->tex_width=gld_GetTexDimension(gltexture->realtexwidth);
    gltexture->tex_height=gld_GetTexDimension(gltexture->realtexheight);
    gltexture->width=min(gltexture->realtexwidth, gltexture->tex_width);
    gltexture->height=min(gltexture->realtexheight, gltexture->tex_height);
    gltexture->textype=GLDT_PATCH;
    R_UnlockPatchNum(lump);
  }
  return gltexture;
}

void gld_BindPatch(GLTexture *gltexture, int cm)
{
  unsigned char *buffer;
  const byte *trans;  

  if ((gltexture==last_gltexture) && (cm==last_cm))
    return;
  last_gltexture=gltexture;
  last_cm=cm;
  if (!gltexture)
    return;
  if (gltexture->textype!=GLDT_PATCH)
  {
    p_glBindTexture(GL_TEXTURE_2D, 0);
    last_gltexture = NULL;
    last_cm = -1;
    return;
  }
  if (gltexture->glTexID[cm]!=0)
  {
    int i;
 	  p_glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
    p_glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_RESIDENT,&i);
#ifdef _DEBUG
    if (i!=GL_TRUE)
      lprintf(LO_INFO, "p_glGetTexParam: %i\n", i);
#endif
    if (i==GL_TRUE)
      return;
  }
  if (cm == CR_DEFAULT)
    trans = NULL;
  else if (cm == CR_LIMIT)
    trans = NULL;
  else if (cm<CR_LIMIT)
    trans = colrngs[cm];
  else
    trans = translationtables[(cm-CR_LIMIT) - 1];;
  buffer = V_GetPlottedPatch32(gltexture->index, gltexture->width, gltexture->height, gltexture->tex_width, gltexture->tex_height, RDRAW_FILTER_POINT, RDRAW_MASKEDCOLUMNEDGE_SLOPED, trans, true);
  if (gltexture->glTexID[cm]==0)
	  p_glGenTextures(1,&gltexture->glTexID[cm]);
	p_glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[cm]);
  p_glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                  gltexture->tex_width, gltexture->tex_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
	p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);
  free(buffer);
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
    gltexture->width=gltexture->tex_width;
    gltexture->height=gltexture->tex_height;
    gltexture->textype=GLDT_FLAT;
  }
  return gltexture;
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
      pos=(y*gltexture->tex_width);
      for (x=0;x<gltexture->realtexwidth;x++,pos++)
      {
        buffer[pos]=gld_palmap[flat[y*64+x]];
      }
    }
  } else {
  playpal=W_CacheLumpName("PLAYPAL");
  for (y=0;y<gltexture->realtexheight;y++)
  {
    pos=4*(y*gltexture->tex_width);
    for (x=0;x<gltexture->realtexwidth;x++,pos+=4)
    {
  		buffer[pos]=playpal[flat[y*64+x]*3];
			buffer[pos+1]=playpal[flat[y*64+x]*3+1];
	  	buffer[pos+2]=playpal[flat[y*64+x]*3+2];
			buffer[pos+3]=255;
    }
  }
  W_UnlockLumpName("PLAYPAL");
  }
}

void gld_BindFlat(GLTexture *gltexture)
{
  const unsigned char *flat;
  int i;
  unsigned char *buffer;
  int buffersize;

  if (gltexture==last_gltexture)
    return;
  last_gltexture=gltexture;
  if (!gltexture)
    return;
  if (gltexture->textype!=GLDT_FLAT)
  {
    p_glBindTexture(GL_TEXTURE_2D, 0);
    last_gltexture = NULL;
    last_cm = -1;
    return;
  }
  if (gltexture->glTexID[CR_DEFAULT]!=0)
  {
   	p_glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
    p_glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_RESIDENT,&i);
#ifdef _DEBUG
    if (i!=GL_TRUE)
      lprintf(LO_INFO, "p_glGetTexParam: %i\n", i);
#endif
    if (i==GL_TRUE)
      return;
  }
  flat=W_CacheLumpNum(gltexture->index);
  buffersize = gltexture->tex_width * gltexture->tex_height * 4;
  buffer=(unsigned char*)Z_Malloc(buffersize,PU_STATIC,0);
  if (!(gltexture->mipmap & use_mipmapping) & gl_paletted_texture)
    memset(buffer,transparent_pal_index,buffersize);
  else
    memset(buffer,0,buffersize);
  gld_AddFlatToTexture(gltexture, buffer, flat, !(gltexture->mipmap & use_mipmapping) & gl_paletted_texture);
  if (gltexture->glTexID[CR_DEFAULT]==0)
  	p_glGenTextures(1,&gltexture->glTexID[CR_DEFAULT]);
	p_glBindTexture(GL_TEXTURE_2D, gltexture->glTexID[CR_DEFAULT]);
/*#ifdef USE_GLU_MIPMAP
  if (gltexture->mipmap & use_mipmapping)
  {
	  p_gluBuild2DMipmaps(GL_TEXTURE_2D, gl_tex_format,
                      gltexture->tex_width, gltexture->tex_height,
					            GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_mipmap_filter);
    if (gl_texture_filter_anisotropic)
      p_glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0);
  }
  else
#endif /* USE_GLU_MIPMAP */
  {
    if (gl_paletted_texture) {
      gld_SetTexturePalette(GL_TEXTURE_2D);
      p_glTexImage2D( GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT,
                    gltexture->tex_width, gltexture->tex_height,
                    0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buffer);
    } else {
      p_glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                    gltexture->tex_width, gltexture->tex_height,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
	  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
	  p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);
  }
  Z_Free(buffer);
  W_UnlockLumpNum(gltexture->index);
}

static void gld_CleanTextures(void)
{
  int i,j;
  int result;

  if (!gld_GLTextures)
    return;
  for (i=0; i<numtextures; i++)
  {
    if (gld_GLTextures[i])
    {
      for (j=0; j<(CR_LIMIT+TRANSLATIONCOLOURS); j++) {
        if (p_glIsTexture(gld_GLTextures[i]->glTexID[j]))
          p_glDeleteTextures(1,&(gld_GLTextures[i]->glTexID[j]));
        result = p_glGetError();
        if (result)
          lprintf(LO_WARN, "glGetError: %i", result);
	  }
      Z_Free(gld_GLTextures[i]);
    }
  }
  memset(gld_GLTextures,0,numtextures*sizeof(GLTexture *));
}

static void gld_CleanPatchTextures(void)
{
  int i,j;
  int result;

  if (!gld_GLPatchTextures)
    return;
  for (i=0; i<numlumps; i++)
  {
    if (gld_GLPatchTextures[i])
    {
      for (j=0; j<(CR_LIMIT+TRANSLATIONCOLOURS); j++) {
        if (p_glIsTexture(gld_GLPatchTextures[i]->glTexID[j]))
          p_glDeleteTextures(1,&(gld_GLPatchTextures[i]->glTexID[j]));
        result = p_glGetError();
        if (result)
          lprintf(LO_WARN, "glGetError: %i", result);
	  }
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
  int i;
  byte *hitlist;

  if (demoplayback)
    return;

  {
    size_t size = numflats > numsprites  ? numflats : numsprites;
    hitlist = malloc((size_t)numtextures > size ? numtextures : size);
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
/*
  {
    thinker_t *th;
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
      if (th->function == P_MobjThinker)
        hitlist[((mobj_t *)th)->sprite] = 1;
  }
*/
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
  free(hitlist);
}

void gld_CleanMemory(void)
{
  gld_CleanTextures();
  gld_CleanPatchTextures();
}
