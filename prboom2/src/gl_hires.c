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

#ifdef _MSC_VER
//#include <ddraw.h> /* needed for DirectX's DDSURFACEDESC2 structure definition */
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_opengl.h>
#ifdef HAVE_LIBSDL_IMAGE
#include <SDL_image.h>
#endif
#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"
#include "i_system.h"
#include "w_wad.h"
#include "lprintf.h"
#include "i_video.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "r_sky.h"
#include "e6y.h"

#ifndef HAVE_STRLWR
char* strlwr(char* str);
#endif

int gl_have_hires_textures = false;
int gl_have_hires_flats = false;
int gl_have_hires_patches = false;
int gl_texture_external_hires = -1;
int gl_texture_internal_hires = -1;
int gl_hires_override_pwads;
char *gl_texture_hires_dir = NULL;

static GLuint progress_texid = 0;

int gld_ProgressStart(void)
{
  if (!progress_texid)
  {
    progress_texid = CaptureScreenAsTexID();
    return true;
  }

  return false;
}

int gld_ProgressRestoreScreen(void)
{
  int total_w, total_h;
  float fU1, fU2, fV1, fV2;

  if (progress_texid)
  {
    total_w = gld_GetTexDimension(SCREENWIDTH);
    total_h = gld_GetTexDimension(SCREENHEIGHT);
    
    fU1 = 0.0f;
    fV1 = (float)SCREENHEIGHT / (float)total_h;
    fU2 = (float)SCREENWIDTH / (float)total_w;
    fV2 = 0.0f;
    
    glEnable(GL_TEXTURE_2D);
    
    glBindTexture(GL_TEXTURE_2D, progress_texid);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    glBegin(GL_TRIANGLE_STRIP);
    {
      glTexCoord2f(fU1, fV1); glVertex2f(0.0f, 0.0f);
      glTexCoord2f(fU1, fV2); glVertex2f(0.0f, (float)SCREENHEIGHT);
      glTexCoord2f(fU2, fV1); glVertex2f((float)SCREENWIDTH, 0.0f);
      glTexCoord2f(fU2, fV2); glVertex2f((float)SCREENWIDTH, (float)SCREENHEIGHT);
    }
    glEnd();

    return true;
  }

  return false;
}

int gld_ProgressEnd(void)
{
  if (progress_texid != 0)
  {
    gld_ProgressRestoreScreen();
    I_FinishUpdate();
    gld_ProgressRestoreScreen();
    glDeleteTextures(1, &progress_texid);
    progress_texid = 0;
    return true;
  }

  return false;
}

void gld_ProgressUpdate(char * text, int progress, int total)
{
  int len;
  static char last_text[32] = {0};
  static unsigned int lastupdate = -1;
  unsigned int tic;

  if (!progress_texid)
    return;

  // do not do it often
  tic = SDL_GetTicks();
  if (tic - lastupdate < 35)
    return;
  lastupdate = tic;

  if ((text) && (strlen(text) > 0) && strcmp((last_text ? last_text : ""), text))
  {
    char *s;
    strcpy(last_text, text);

    if (!w_precache.f)
      HU_Start();

    HUlib_clearTextLine(&w_precache);
    s = text;
    while (*s)
      HUlib_addCharToTextLine(&w_precache, *(s++));
    HUlib_setTextXCenter(&w_precache);
  }

  gld_ProgressRestoreScreen();
  HUlib_drawTextLine(&w_precache, false);
  
  len = MIN(SCREENWIDTH, (int)((int_64_t)SCREENWIDTH * progress / total));
  V_FillRect(0, 0, SCREENHEIGHT - 4, len - 0, 4, 4);
  if (len > 4)
  {
    V_FillRect(0, 2, SCREENHEIGHT - 3, len - 4, 2, 31);
  }

  I_FinishUpdate();
}

#ifdef HAVE_LIBSDL_IMAGE

static const char* gld_HiRes_GetInternalName(GLTexture *gltexture);
static int gld_HiRes_GetExternalName(GLTexture *gltexture, char *img_path, char *dds_path);
static void gld_HiRes_Bind(GLTexture *gltexture, int *glTexID);
static int gld_HiRes_LoadInternal(GLTexture *gltexture, int *glTexID);
static int gld_HiRes_LoadExternal(GLTexture *gltexture, int *glTexID);

#define DDRAW_H_MAKEFOURCC(ch0, ch1, ch2, ch3) \
  ((unsigned long)(unsigned char)(ch0) | ((unsigned long)(unsigned char)(ch1) << 8) | \
  ((unsigned long)(unsigned char)(ch2) << 16) | ((unsigned long)(unsigned char)(ch3) << 24 ))

/*
 * FOURCC codes for DX compressed-texture pixel formats
 */
#define DDRAW_H_FOURCC_DXT1 (DDRAW_H_MAKEFOURCC('D','X','T','1'))
#define DDRAW_H_FOURCC_DXT2 (DDRAW_H_MAKEFOURCC('D','X','T','2'))
#define DDRAW_H_FOURCC_DXT3 (DDRAW_H_MAKEFOURCC('D','X','T','3'))
#define DDRAW_H_FOURCC_DXT4 (DDRAW_H_MAKEFOURCC('D','X','T','4'))
#define DDRAW_H_FOURCC_DXT5 (DDRAW_H_MAKEFOURCC('D','X','T','5'))

/*
 * DDSCAPS2
 */
typedef struct _DDRAW_H_DDSCAPS2
{
  unsigned long dwCaps;     // capabilities of surface wanted
  unsigned long dwCaps2;
  unsigned long dwCaps3;
  union
  {
    unsigned long dwCaps4;
    unsigned long dwVolumeDepth;
  } u1;
} DDRAW_H_DDSCAPS2;

/*
 * DDPIXELFORMAT
 */
typedef struct _DDRAW_H_DDPIXELFORMAT
{
  unsigned long dwSize;         // size of structure
  unsigned long dwFlags;        // pixel format flags
  unsigned long dwFourCC;       // (FOURCC code)
  union
  {
    unsigned long dwRGBBitCount;           // how many bits per pixel
    unsigned long dwYUVBitCount;           // how many bits per pixel
    unsigned long dwZBufferBitDepth;       // how many total bits/pixel in z buffer (including any stencil bits)
    unsigned long dwAlphaBitDepth;         // how many bits for alpha channels
    unsigned long dwLuminanceBitCount;     // how many bits per pixel
    unsigned long dwBumpBitCount;          // how many bits per "buxel", total
    unsigned long dwPrivateFormatBitCount; // Bits per pixel of private driver formats. Only valid in texture
                                           // format list and if DDPF_D3DFORMAT is set
  } u1;
  union
  {
    unsigned long dwRBitMask;         // mask for red bit
    unsigned long dwYBitMask;         // mask for Y bits
    unsigned long dwStencilBitDepth;  // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
    unsigned long dwLuminanceBitMask; // mask for luminance bits
    unsigned long dwBumpDuBitMask;    // mask for bump map U delta bits
    unsigned long dwOperations;       // DDPF_D3DFORMAT Operations
  } u2;
  union
  {
    unsigned long dwGBitMask;      // mask for green bits
    unsigned long dwUBitMask;      // mask for U bits
    unsigned long dwZBitMask;      // mask for Z bits
    unsigned long dwBumpDvBitMask; // mask for bump map V delta bits
    struct
    {
      unsigned short wFlipMSTypes; // Multisample methods supported via flip for this D3DFORMAT
      unsigned short wBltMSTypes;  // Multisample methods supported via blt for this D3DFORMAT
    } MultiSampleCaps;

  } u3;
  union
  {
    unsigned long dwBBitMask;             // mask for blue bits
    unsigned long dwVBitMask;             // mask for V bits
    unsigned long dwStencilBitMask;       // mask for stencil bits
    unsigned long dwBumpLuminanceBitMask; // mask for luminance in bump map
  } u4;
  union
  {
    unsigned long dwRGBAlphaBitMask;       // mask for alpha channel
    unsigned long dwYUVAlphaBitMask;       // mask for alpha channel
    unsigned long dwLuminanceAlphaBitMask; // mask for alpha channel
    unsigned long dwRGBZBitMask;           // mask for Z channel
    unsigned long dwYUVZBitMask;           // mask for Z channel
  } u5;
} DDRAW_H_DDPIXELFORMAT;

/*
 * DDCOLORKEY
 */
typedef struct _DDRAW_H_DDCOLORKEY
{
  unsigned long dwColorSpaceLowValue;  // low boundary of color space that is to
                                       // be treated as Color Key, inclusive
  unsigned long dwColorSpaceHighValue; // high boundary of color space that is
} DDRAW_H_DDCOLORKEY;

/*
 * DDSURFACEDESC2
 */
typedef struct _DDRAW_H_DDSURFACEDESC2
{
  unsigned long dwSize;   // size of the DDSURFACEDESC structure
  unsigned long dwFlags;  // determines what fields are valid
  unsigned long dwHeight; // height of surface to be created
  unsigned long dwWidth;  // width of input surface
  union
  {
    long          lPitch;       // distance to start of next line (return value only)
    unsigned long dwLinearSize; // Formless late-allocated optimized surface size
  } u1;
  union
  {
    unsigned long dwBackBufferCount; // number of back buffers requested
    unsigned long dwDepth;           // the depth if this is a volume texture 
  } u5;
  union
  {
    unsigned long dwMipMapCount; // number of mip-map levels requestde
                                 // dwZBufferBitDepth removed, use ddpfPixelFormat one instead
    unsigned long dwRefreshRate; // refresh rate (used when display mode is described)
    unsigned long dwSrcVBHandle; // The source used in VB::Optimize
  } u2;
  unsigned long dwAlphaBitDepth; // depth of alpha buffer requested
  unsigned long dwReserved;      // reserved
  void *lpSurface;               // pointer to the associated surface memory
  union
  {
    DDRAW_H_DDCOLORKEY ddckCKDestOverlay; // color key for destination overlay use
    unsigned long      dwEmptyFaceColor;  // Physical color for empty cubemap faces
  } u3;
  DDRAW_H_DDCOLORKEY ddckCKDestBlt;    // color key for destination blt use
  DDRAW_H_DDCOLORKEY ddckCKSrcOverlay; // color key for source overlay use
  DDRAW_H_DDCOLORKEY ddckCKSrcBlt;     // color key for source blt use
  union
  {
    DDRAW_H_DDPIXELFORMAT ddpfPixelFormat; // pixel format description of the surface
    unsigned long  dwFVF;                  // vertex format description of vertex buffers
  } u4;
  DDRAW_H_DDSCAPS2 ddsCaps;        // direct draw surface capabilities
  unsigned long    dwTextureStage; // stage in multitexture cascade
} DDRAW_H_DDSURFACEDESC2;

typedef struct
{
  GLsizei  width;
  GLsizei  height;
  GLint    components;
  GLenum   format;

  GLsizei  cmapEntries;
  GLenum   cmapFormat;
  GLubyte *cmap;

  GLubyte *pixels;
} GLGenericImage;

GLGenericImage * ReadDDSFile(const char *filename, int * bufsize, int * numMipmaps) 
{
  GLGenericImage * genericImage;
  DDRAW_H_DDSURFACEDESC2 ddsd;
  char filecode[4];
  FILE *fp;
  int factor;

  /* try to open the file */
  fp = fopen(filename, "rb");
  if (fp == NULL)
    return NULL;
  
  /* verify the type of file */
  fread(filecode, 1, 4, fp);
  if (strncmp(filecode, "DDS ", 4) != 0) {
    fclose(fp);
    return NULL;
  }
   
  /* get the surface desc */
  fread(&ddsd, sizeof(ddsd), 1, fp);

  genericImage = (GLGenericImage*)malloc(sizeof(GLGenericImage));
  memset(genericImage, 0, sizeof(GLGenericImage));

  switch(ddsd.u4.ddpfPixelFormat.dwFourCC)
  {
    case DDRAW_H_FOURCC_DXT1:
      genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      factor = 2;
      break;
    case DDRAW_H_FOURCC_DXT3:
      genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
      factor = 4;
      break;
    case DDRAW_H_FOURCC_DXT5:
      genericImage->format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      factor = 4;
      break;
    default:
      return NULL;
  }

  /* how big is it going to be including all mipmaps? */
  *bufsize = ddsd.u2.dwMipMapCount > 1 ? ddsd.u1.dwLinearSize * factor : ddsd.u1.dwLinearSize;
  genericImage->pixels = (unsigned char*)malloc(*bufsize * sizeof(unsigned char));
  fread(genericImage->pixels, 1, *bufsize, fp);
  /* close the file pointer */
  fclose(fp);
  
  genericImage->width       = ddsd.dwWidth;
  genericImage->height      = ddsd.dwHeight;
  genericImage->components  = (ddsd.u4.ddpfPixelFormat.dwFourCC == DDRAW_H_FOURCC_DXT1) ? 3 : 4;
  *numMipmaps               = ddsd.u2.dwMipMapCount;

  /* return data */
  return genericImage;
}

static SDL_PixelFormat RGBAFormat;

static const char* gld_HiRes_GetInternalName(GLTexture *gltexture)
{
  static char texname[9];
  char *texname_p = NULL;

  switch (gltexture->textype)
  {
  case GLDT_TEXTURE:
    texname_p = textures[gltexture->index]->name;
    break;
  case GLDT_FLAT:
  case GLDT_PATCH:
    texname_p = lumpinfo[gltexture->index].name;
    break;
  }

  if (!texname_p)
    return NULL;

  strncpy(texname, texname_p, 8);
  texname[8] = 0;
  strlwr(texname);

  return texname;
}

static int gld_HiRes_GetExternalName(GLTexture *gltexture, char *img_path, char *dds_path)
{
  typedef struct hires_path_item_s
  {
    const char *path;
    int exists;
  } hires_path_item_t;

  typedef struct hires_path_s
  {
    const GameMission_t gamemission;
    const GLTexType textype;
    hires_path_item_t item[16];
  } hires_path_t;

  
  static hires_path_t hires_paths[] = {
    {doom, GLDT_TEXTURE, {
      {"%stextures/doom/doom1/%s.%s", -1},
      {"%stextures/doom/doom1/%s-ck.%s", -1},
      {"%stextures/doom1/%s.%s", -1},
      {"%stextures/doom/%s.%s", -1},
      {"%stextures/doom/%s-ck.%s", -1},
      {"%stextures/%s.%s", -1},
      {"%stextures/%s-ck.%s", -1},
      {NULL, 0}
    }},
    {doom, GLDT_FLAT, {
      {"%sflats/doom/doom1/%s.%s", -1},
      {"%stextures/doom/doom1/flat-%s.%s", -1},
      {"%stextures/doom1/flat-%s.%s", -1},
      {"%sflats/doom/%s.%s", -1},
      {"%stextures/doom/flat-%s.%s", -1},
      {"%sflats/%s.%s", -1},
      {"%stextures/flat-%s.%s", -1},
      {NULL}
    }},
    {doom, GLDT_PATCH, {
      {"%spatches/doom/doom1/%s.%s", -1},
      {"%spatches/doom1-ultimate/%s.%s", -1},
      {"%spatches/doom/%s.%s", -1},
      {"%spatches/%s.%s", -1},
      {NULL}
    }},

    {doom2, GLDT_TEXTURE, {
      {"%stextures/doom/doom2/%s.%s", -1},
      {"%stextures/doom/doom2/%s-ck.%s", -1},
      {"%stextures/doom/%s.%s", -1},
      {"%stextures/doom/%s-ck.%s", -1},
      {"%stextures/%s.%s", -1},
      {"%stextures/%s-ck.%s", -1},
      {NULL}
    }},
    {doom2, GLDT_FLAT, {
      {"%sflats/doom/doom2/%s.%s", -1},
      {"%stextures/doom/doom2/flat-%s.%s", -1},
      {"%sflats/doom/%s.%s", -1},
      {"%stextures/doom/flat-%s.%s", -1},
      {"%sflats/%s.%s", -1},
      {"%stextures/flat-%s.%s", -1},
      {NULL}
    }},
    {doom2, GLDT_PATCH, {
      {"%spatches/doom/doom2/%s.%s", -1},
      {"%spatches/doom2/%s.%s", -1},
      {"%spatches/doom/%s.%s", -1},
      {"%spatches/%s.%s", -1},
      {NULL}
    }},

    {pack_tnt, GLDT_TEXTURE, {
      {"%stextures/doom/tnt/%s.%s", -1},
      {"%stextures/doom/tnt/%s-ck.%s", -1},
      {"%stextures/doom2-tnt/%s.%s", -1},
      {"%stextures/doom/doom2-tnt/%s.%s", -1},
      {"%stextures/doom/doom2-tnt/%s-ck.%s", -1},
      {"%stextures/doom/%s.%s", -1},
      {"%stextures/doom/%s-ck.%s", -1},
      {"%stextures/%s.%s", -1},
      {"%stextures/%s-ck.%s", -1},
      {NULL}
    }},
    {pack_tnt, GLDT_FLAT, {
      {"%sflats/doom/tnt/%s.%s", -1},
      {"%stextures/doom/tnt/flat-%s.%s", -1},
      {"%sflats/doom/doom2-tnt/%s.%s", -1},
      {"%stextures/doom/doom2-tnt/flat-%s.%s", -1},
      {"%stextures/doom2-tnt/flat-%s.%s", -1},
      {"%sflats/doom/%s.%s", -1},
      {"%stextures/doom/flat-%s.%s", -1},
      {"%sflats/%s.%s", -1},
      {"%stextures/flat-%s.%s", -1},
      {NULL}
    }},
    {pack_tnt, GLDT_PATCH, {
      {"%spatches/doom/tnt/%s.%s", -1},
      {"%spatches/doom2-tnt/%s.%s", -1},
      {"%spatches/tnt/%s.%s", -1},
      {"%spatches/doom/%s.%s", -1},
      {"%spatches/%s.%s", -1},
      {NULL}
    }},

    {pack_plut, GLDT_TEXTURE, {
      {"%stextures/doom/plut/%s.%s", -1},
      {"%stextures/doom/plut/%s-ck.%s", -1},
      {"%stextures/doom2-plut/%s.%s", -1},
      {"%stextures/doom/doom2-plut/%s.%s", -1},
      {"%stextures/doom/doom2-plut/%s-ck.%s", -1},
      {"%stextures/doom/%s.%s", -1},
      {"%stextures/doom/%s-ck.%s", -1},
      {"%stextures/%s.%s", -1},
      {"%stextures/%s-ck.%s", -1},
      {NULL}
    }},
    {pack_plut, GLDT_FLAT, {
      {"%sflats/doom/plut/%s.%s", -1},
      {"%stextures/doom/plut/flat-%s.%s", -1},
      {"%sflats/doom/doom2-plut/%s.%s", -1},
      {"%stextures/doom/doom2-plut/flat-%s.%s", -1},
      {"%stextures/doom2-plut/flat-%s.%s", -1},
      {"%sflats/doom/%s.%s", -1},
      {"%stextures/doom/flat-%s.%s", -1},
      {"%sflats/%s.%s", -1},
      {"%stextures/flat-%s.%s", -1},
      NULL
    }},
    {pack_plut, GLDT_PATCH, {
      {"%spatches/doom/plut/%s.%s", -1},
      {"%spatches/doom2-plut/%s.%s", -1},
      {"%spatches/plutonia/%s.%s", -1},
      {"%spatches/doom/%s.%s", -1},
      {"%spatches/%s.%s", -1},
      {NULL}
    }},

    {none, GLDT_UNREGISTERED, {
      NULL
    }},
  };

  static char *hiresdir = NULL;

  char texname[9];
  char *texname_p = NULL;
  int i;

  hires_path_item_t *checklist = NULL;
  GLTexType useType = gltexture->textype;

  DOOM_BOOL supported = (gl_texture_external_hires && 
    ((useType == GLDT_TEXTURE) || (useType == GLDT_FLAT) || (useType == GLDT_PATCH)));

  img_path[0] = '\0';
  dds_path[0] = '\0';

  if (!supported)
    return false;

  i = 0;
  while (hires_paths[i].gamemission != none)
  {
    if (gamemission == hires_paths[i].gamemission && 
      gltexture->textype == hires_paths[i].textype)
    {
      checklist = &hires_paths[i].item[0];
      break;
    }
    i++;
  }
  if (!checklist)
    return false;

  switch (useType)
  {
  case GLDT_TEXTURE:
    {
      int i;
      texture_t *texture = textures[gltexture->index];

      if (!gl_hires_override_pwads)
      {
        for (i = 0; i < texture->patchcount; i++)
        {
          if (lumpinfo[texture->patches[i].patch].source != source_iwad)
            return false;
        }
      }
      texname_p = texture->name;
    }
    break;
  case GLDT_FLAT:
  case GLDT_PATCH:
    {
      if (!gl_hires_override_pwads)
      {
        if (lumpinfo[gltexture->index].source != source_iwad)
          return false;
      }
      texname_p = lumpinfo[gltexture->index].name;
    }
    break;
  }

  if (!texname_p)
    return false;

  strncpy(texname, texname_p, 8);
  texname[8] = 0;
  strlwr(texname);

  if (!hiresdir)
  {
    hiresdir = malloc(PATH_MAX + 1);
    if (strlen(gl_texture_hires_dir) > 0)
    {
      strncpy(hiresdir, gl_texture_hires_dir, PATH_MAX - 1);
    }
    else
    {
      strncpy(hiresdir, I_DoomExeDir(), PATH_MAX - 1);
    }
    if (!HasTrailingSlash(hiresdir))
      strcat(hiresdir, "/");
  }

  do
  {
    char checkName[PATH_MAX + 1];
    
    if (checklist->exists == 0)
      continue;
    
    if (checklist->exists == -1)
    {
      SNPRINTF(checkName, sizeof(checkName), checklist->path, hiresdir, "", "");
      if (!access(checkName, F_OK))
        checklist->exists = 1;
      else
        checklist->exists = 0;
    }
    
    if (checklist->exists == 1) //dir exists
    {
      if (GLEXT_glCompressedTexImage2DARB && dds_path[0] == '\0')
      {
        SNPRINTF(checkName, sizeof(checkName), checklist->path, hiresdir, texname, "dds");
        if (!access(checkName, F_OK))
        {
          strcpy(dds_path, checkName);
        }
      }
      
      {
        static const char * extensions[] =
        {"png", "jpg", "tga", "pcx", "gif", "bmp", NULL};
        const char ** extp;
        
        for (extp = extensions; *extp; extp++)
        {
          SNPRINTF(checkName, sizeof(checkName), checklist->path, hiresdir, texname, *extp);
          
          if (!access(checkName, F_OK))
          {
            strcpy(img_path, checkName);
            return true;
          }
        }
      }
    }
  } while ((++checklist)->path);

  if (dds_path[0])
    return true;

  return false;
}

static void gld_HiRes_Bind(GLTexture *gltexture, int *glTexID)
{
  switch (gltexture->textype)
  {
  case GLDT_TEXTURE:
    gl_have_hires_textures = true;
    break;
  case GLDT_FLAT:
    gl_have_hires_flats = true;
    break;
  case GLDT_PATCH:
    gl_have_hires_patches = true;
    break;
  }

  gltexture->mipmap = 
    ((gltexture->textype == GLDT_TEXTURE) || 
    (gltexture->textype == GLDT_FLAT));

  gltexture->flags |= GLTEXTURE_HIRES;

  if (gltexture->textype == GLDT_PATCH)
  {
    gltexture->scalexfac = 1.0f;
    gltexture->scaleyfac = 1.0f;
  }

  if (*glTexID == 0)
    glGenTextures(1, glTexID);

  glBindTexture(GL_TEXTURE_2D, *glTexID);
}

static int gld_HiRes_LoadInternal(GLTexture *gltexture, int *glTexID)
{
  int result = false;
  const char *lumpname;

  lumpname = gld_HiRes_GetInternalName(gltexture);

  if (lumpname)
  {
    int lump = (W_CheckNumForName)(lumpname, ns_hires);
    if (lump != -1)
    {
      SDL_RWops *rw_data = SDL_RWFromMem((void*)W_CacheLumpNum(lump), W_LumpLength(lump));
      SDL_Surface *surf_tmp = IMG_Load_RW(rw_data, true);

      if (surf_tmp)
      {
        SDL_Surface *surf = SDL_ConvertSurface(surf_tmp, &RGBAFormat, surf_tmp->flags);
        SDL_FreeSurface(surf_tmp);

        if (surf)
        {
          gld_GammaCorrect(surf->pixels, surf->pitch * surf->h);

          gld_HiRes_Bind(gltexture, glTexID);

          result = gld_BuildTexture(gltexture, surf->pixels, true,
            surf->pitch, surf->w, surf->h, NULL, NULL, NULL, NULL);

          SDL_FreeSurface(surf);
        }
      }
    }
  }

  return result;
}

static int gld_HiRes_LoadExternal(GLTexture *gltexture, int *glTexID)
{
  int result = false;

  SDL_Surface *surf = NULL;
  SDL_Surface *surf_tmp = NULL;
  int tex_width, tex_height, tex_buffer_size;
  unsigned char *tex_buffer = NULL;

  char img_path[PATH_MAX];
  char dds_path[PATH_MAX];
  DOOM_BOOL img_present, dds_present;
  
  char cache_filename[PATH_MAX];
  FILE *cachefp = NULL;
  int cache_write = false;
  int cache_read = false;
  int cache_read_ok = false;

  struct stat tex_stat;

  if (!gld_HiRes_GetExternalName(gltexture, img_path, dds_path))
    return false;

  img_present = strlen(img_path) > 4;
  dds_present = strlen(dds_path) > 4;

  if (!img_present && !dds_present)
    return false;

  if (GLEXT_glCompressedTexImage2DARB && dds_present)
  {
    int ddsbufsize, numMipmaps;
    GLGenericImage * ddsimage = ReadDDSFile(dds_path, &ddsbufsize, &numMipmaps);

    if (ddsimage)
    {
      tex_width  = gld_GetTexDimension(ddsimage->width);
      tex_height = gld_GetTexDimension(ddsimage->height);

      if (tex_width == ddsimage->width && tex_height == ddsimage->height)
      {
        int i, offset, size, blockSize;

        gld_HiRes_Bind(gltexture, glTexID);

        offset = 0;
        blockSize = (ddsimage->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

        /* load the mipmaps */
        for (i = 0; i < (numMipmaps ? numMipmaps : 1) && (ddsimage->width || ddsimage->height); i++)
        {
          if (ddsimage->width == 0)
            ddsimage->width = 1;
          if (ddsimage->height == 0)
            ddsimage->height = 1;
      
          size = ((ddsimage->width + 3) / 4) * ((ddsimage->height + 3) / 4) * blockSize;
      
          GLEXT_glCompressedTexImage2DARB(GL_TEXTURE_2D, i, ddsimage->format,
            ddsimage->width, ddsimage->height, 
            0, size, ddsimage->pixels + offset);
      
    //      GLErrorReport();
          offset += size;
          ddsimage->width >>= 1;
          ddsimage->height >>= 1;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gltexture->wrap_mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gltexture->wrap_mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
          (numMipmaps ? gl_mipmap_filter : gl_tex_filter));
        if (gl_ext_texture_filter_anisotropic && numMipmaps)
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)(1<<gl_texture_filter_anisotropic));
        
        free(ddsimage->pixels);
        free(ddsimage);

        result = true;
        goto l_exit;
      }
    }
  }

  if (!img_present)
  {
    result = false;
    goto l_exit;
  }

  memset(&tex_stat, 0, sizeof(tex_stat));
  stat(img_path, &tex_stat);
  
  strcpy(cache_filename, img_path);
  strcat(cache_filename, ".cache");

  if (!access(cache_filename, F_OK))
  {
    cache_read = !access(cache_filename, R_OK);
  }
  else
  {
    cache_write = 
      ((gltexture->realtexwidth != gltexture->tex_width) ||
      (gltexture->realtexheight != gltexture->tex_height));
  }

  if (cache_read)
  {
    struct stat cache_stat;

    cache_read_ok = false;

    tex_width = 0;
    tex_height = 0;

    cachefp = fopen(cache_filename, "rb");

    if (cachefp)
    {
      if (fread(&tex_width, sizeof(tex_width), 1, cachefp) == 1 &&
        fread(&tex_height, sizeof(tex_height), 1, cachefp) == 1 &&
        fread(&cache_stat.st_mtime, sizeof(cache_stat.st_mtime), 1, cachefp) == 1)
      {
        if (cache_stat.st_mtime == tex_stat.st_mtime)
        {
          tex_buffer_size = tex_width * tex_height * 4;

          tex_buffer = malloc(tex_buffer_size);
          if (tex_buffer)
          {
            if (fread(tex_buffer, tex_buffer_size, 1, cachefp) == 1)
            {
              gld_HiRes_Bind(gltexture, glTexID);

              gld_GammaCorrect(tex_buffer, gltexture->buffer_size);

              gld_BuildTexture(gltexture, tex_buffer, true,
                tex_width, tex_width, tex_height,
                NULL, NULL, NULL, NULL);

              cache_read_ok = true;
            }
            free(tex_buffer);
          }
        }
      }
      fclose(cachefp);
    }
  }

  if (!cache_read_ok)
  {
    surf_tmp = IMG_Load(img_path);

    if (!surf_tmp)
    {
      lprintf(LO_ERROR, "gld_HiRes_LoadExternal: ");
      lprintf(LO_ERROR, SDL_GetError());
      lprintf(LO_ERROR, "\n");
      return false;
    }

    surf = SDL_ConvertSurface(surf_tmp, &RGBAFormat, surf_tmp->flags);
    SDL_FreeSurface(surf_tmp);

    if (!surf)
      return false;

    gld_GammaCorrect(surf->pixels, surf->pitch * surf->h);

    gld_HiRes_Bind(gltexture, glTexID);

    result = gld_BuildTexture(gltexture, surf->pixels, true,
      surf->pitch, surf->w, surf->h,
      &tex_buffer, &tex_buffer_size, &tex_width, &tex_height);

    if (tex_buffer && (cache_write || !cache_read_ok))
    {
      cachefp = fopen(cache_filename, "wb");
      if (cachefp)
      {
        fwrite(&tex_width, sizeof(tex_width), 1, cachefp);
        fwrite(&tex_height, sizeof(tex_height), 1, cachefp);
        fwrite(&tex_stat.st_mtime, sizeof(tex_stat.st_mtime), 1, cachefp);
        fwrite(tex_buffer, tex_buffer_size, 1, cachefp);
        fclose(cachefp);
      }

      if (tex_buffer != surf->pixels)
      {
        free(tex_buffer);
      }
    }
  }

  result = true;
  goto l_exit;

l_exit:
  if (surf)
    SDL_FreeSurface(surf);

  return result;
}

void gld_InitHiRes(void)
{
  //Init RGBAFormat
  RGBAFormat.palette = 0;
  RGBAFormat.colorkey = 0;
  RGBAFormat.alpha = 0;
  RGBAFormat.BitsPerPixel = 32;
  RGBAFormat.BytesPerPixel = 4;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  RGBAFormat.Rmask = 0xFF000000; RGBAFormat.Rshift = 0; RGBAFormat.Rloss = 0;
  RGBAFormat.Gmask = 0x00FF0000; RGBAFormat.Gshift = 8; RGBAFormat.Gloss = 0;
  RGBAFormat.Bmask = 0x0000FF00; RGBAFormat.Bshift = 16; RGBAFormat.Bloss = 0;
  RGBAFormat.Amask = 0x000000FF; RGBAFormat.Ashift = 24; RGBAFormat.Aloss = 0;
#else
  RGBAFormat.Rmask = 0x000000FF; RGBAFormat.Rshift = 24; RGBAFormat.Rloss = 0;
  RGBAFormat.Gmask = 0x0000FF00; RGBAFormat.Gshift = 16; RGBAFormat.Gloss = 0;
  RGBAFormat.Bmask = 0x00FF0000; RGBAFormat.Bshift = 8; RGBAFormat.Bloss = 0;
  RGBAFormat.Amask = 0xFF000000; RGBAFormat.Ashift = 0; RGBAFormat.Aloss = 0;
#endif

  gl_have_hires_textures = false;
  gl_have_hires_flats = false;
  gl_have_hires_patches = false;

  gld_PrecachePatches();
}

int gld_LoadHiresTex(GLTexture *gltexture, int *glTexID, int cm)
{
  if (!gl_texture_external_hires && !gl_texture_internal_hires)
    return false;

  if (gl_texture_internal_hires && gld_HiRes_LoadInternal(gltexture, glTexID))
    return true;

  if (gl_texture_external_hires && gld_HiRes_LoadExternal(gltexture, glTexID))
    return true;

  return false;
}

static void gld_Mark_CM2RGB_Lump(const char *name)
{
  int lump = W_CheckNumForName(name);
  if (lump > 0)
  {
    lumpinfo[lump].flags |= LUMP_CM2RGB;
  }
}

int gld_PrecachePatches(void)
{
  static const char * staticpatches[] = {
    "INTERPIC",// "TITLEPIC",

    //doom's M_*
    "M_DETAIL", "M_DISOPT", "M_DISP",   "M_DOOM",
    "M_ENDGAM", "M_EPI1",   "M_EPI2",   "M_EPI3",
    "M_EPISOD", "M_GDHIGH", "M_GDLOW",  "M_HURT",
    "M_JKILL",  "M_LGTTL",  "M_LOADG",  "M_LSCNTR",
    "M_LSLEFT", "M_LSRGHT", "M_MESSG",  "M_MSENS",
    "M_MSGOFF", "M_MSGON",  "M_MUSVOL", "M_NEWG",
    "M_NGAME",  "M_NMARE",  "M_OPTION", "M_OPTTTL",
    "M_PAUSE",  "M_QUITG",  "M_RDTHIS", "M_ROUGH",
    "M_SAVEG",  "M_SCRNSZ", "M_SFXVOL", "M_SGTTL",
    "M_SKILL",  "M_SKULL1", "M_SKULL2", "M_SVOL",
    "M_THERML", "M_THERMM", "M_THERMO", "M_THERMR",
    "M_ULTRA",  
    "M_EPI4",

    //prboom's M_*
    "M_ABOUT",  "M_ACCEL",  "M_AUTO",   "M_BUTT1",
    "M_BUTT2",  "M_CHAT",   "M_COLORS", "M_COMPAT",
    "M_COMPAT", "M_DEMOS",  "M_ENEM",   "M_FEAT",
    "M_GENERL", "M_HORSEN", "M_HUD",    "M_KEYBND",
    "M_KEYBND", "M_LOKSEN", "M_MESS",   "M_MOUSE",
    "M_MULTI",  "M_PALNO",  "M_PALSEL", "M_SETUP",
    "M_SLIDEL", "M_SLIDEM", "M_SLIDEO", "M_SLIDER",
    "M_SOUND",  "M_STAT",   "M_STAT",   "M_VBOX",
    "M_VERSEN", "M_VIDEO",  "M_WAD",    "M_WEAP",

    NULL
  };

  const char ** patch_p;
  char namebuf[16];
  int i, count, total;

  if (!gl_texture_external_hires)
    return 0;

  gld_ProgressStart();

  count = 0; total = 0;
  for (patch_p = staticpatches; *patch_p; patch_p++)
    total++;

  for (patch_p = staticpatches; *patch_p; patch_p++)
  {
    int lump = W_CheckNumForName(*patch_p);
    if (lump > 0)
    {
      GLTexture *gltexture;
      
      lumpinfo[lump].flags |= LUMP_STATIC;
      gltexture = gld_RegisterPatch(lump, CR_DEFAULT);
      if (gltexture)
      {
        gld_PrecacheGLTexture(gltexture);
        if (gltexture && (gltexture->flags & GLTEXTURE_HIRES))
        {
          gld_ProgressUpdate("Loading Patches...", ++count, total);
        }
      }
    }
  }

  for (i = 33; i < 96; i++)
  {
    sprintf(namebuf, "STCFN%.3d", i);
    gld_Mark_CM2RGB_Lump(namebuf);
  }
  for (i = 0; i < 10; i++)
  {
    sprintf(namebuf, "STTNUM%d", i);
    gld_Mark_CM2RGB_Lump(namebuf);
  }
  gld_Mark_CM2RGB_Lump("STTPRCNT");

  gld_ProgressEnd();

  return 0;
}

#endif // HAVE_LIBSDL_IMAGE
