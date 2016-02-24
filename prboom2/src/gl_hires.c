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

#ifdef _MSC_VER
//#include <ddraw.h> /* needed for DirectX's DDSURFACEDESC2 structure definition */
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <SDL.h>
#ifdef HAVE_LIBSDL2_IMAGE
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
#include "r_main.h"
#include "r_sky.h"
#include "m_argv.h"
#include "m_misc.h"
#include "e6y.h"

unsigned int gl_has_hires = 0;
int gl_texture_external_hires = -1;
int gl_texture_internal_hires = -1;
int gl_hires_override_pwads;
const char *gl_texture_hires_dir = NULL;
int gl_hires_24bit_colormap = false;

static GLuint progress_texid = 0;
static unsigned int lastupdate = 0;

int gld_ProgressStart(void)
{
  if (!progress_texid)
  {
    progress_texid = CaptureScreenAsTexID();
    lastupdate = SDL_GetTicks() - 100;
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
    
    gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
    
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

void gld_ProgressUpdate(const char * text, int progress, int total)
{
  int len;
  static char last_text[32] = {0};
  unsigned int tic;

  if (!progress_texid)
    return;

  // do not do it often
  tic = SDL_GetTicks();
  if (tic - lastupdate < 100)
    return;
  lastupdate = tic;

  if ((text) && (strlen(text) > 0) && strcmp((last_text[0] ? last_text : ""), text))
  {
    const char *s;
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

#ifdef HAVE_LIBSDL2_IMAGE

static const char* gld_HiRes_GetInternalName(GLTexture *gltexture);
static int gld_HiRes_GetExternalName(GLTexture *gltexture, char *img_path, char *dds_path);
static void gld_HiRes_Bind(GLTexture *gltexture, GLuint *glTexID);

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
  GLGenericImage * genericImage = NULL;
  DDRAW_H_DDSURFACEDESC2 ddsd;
  char filecode[4];
  FILE *fp = NULL;
  int factor;
  int result = false;

  /* try to open the file */
  fp = fopen(filename, "rb");
  if (fp != NULL)
  {
    if ((fread(filecode, 4, 1, fp) == 1) &&
      (strncmp(filecode, "DDS ", 4) == 0) &&    // verify the type of file
      (fread(&ddsd, sizeof(ddsd), 1, fp) == 1)) // get the surface desc
    {
      genericImage = malloc(sizeof(GLGenericImage));
      if (genericImage)
      {
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
          factor = -1;
          break;
        }

        if (factor != -1)
        {
          /* how big is it going to be including all mipmaps? */
          *bufsize = ddsd.u2.dwMipMapCount > 1 ? ddsd.u1.dwLinearSize * factor : ddsd.u1.dwLinearSize;
          genericImage->pixels = malloc(*bufsize * sizeof(unsigned char));

          if (fread(genericImage->pixels, 1, *bufsize, fp) > 0)
          {
            genericImage->width       = ddsd.dwWidth;
            genericImage->height      = ddsd.dwHeight;
            genericImage->components  = (ddsd.u4.ddpfPixelFormat.dwFourCC == DDRAW_H_FOURCC_DXT1) ? 3 : 4;
            *numMipmaps               = ddsd.u2.dwMipMapCount;

            result = true;
          }
        }
      }
    }

    /* close the file pointer */
    fclose(fp);
  }

  if (result)
  {
    /* return data */
    return genericImage;
  }
  else
  {
    if (genericImage)
      free(genericImage);

    return NULL;
  }
}

static byte* RGB2PAL = NULL;

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
  M_Strlwr(texname);

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
      {NULL}
    }},
    {pack_plut, GLDT_PATCH, {
      {"%spatches/doom/plut/%s.%s", -1},
      {"%spatches/doom2-plut/%s.%s", -1},
      {"%spatches/plutonia/%s.%s", -1},
      {"%spatches/doom/%s.%s", -1},
      {"%spatches/%s.%s", -1},
      {NULL}
    }},

    {hacx, GLDT_TEXTURE, {
      {"%stextures/hacx/%s.%s", -1},
      {"%stextures/hacx/%s-ck.%s", -1},
      {"%stextures/%s.%s", -1},
      {"%stextures/%s-ck.%s", -1},
      {NULL}
    }},
    {hacx, GLDT_FLAT, {
      {"%sflats/hacx/%s.%s", -1},
      {"%stextures/hacx/flat-%s.%s", -1},
      {"%sflats/%s.%s", -1},
      {"%stextures/flat-%s.%s", -1},
      {NULL}
    }},
    {hacx, GLDT_PATCH, {
      {"%spatches/hacx/%s.%s", -1},
      {"%spatches/%s.%s", -1},
      {NULL}
    }},

    {chex, GLDT_TEXTURE, {
      {"%stextures/chex/%s.%s", -1},
      {"%stextures/chex/%s-ck.%s", -1},
      {"%stextures/%s.%s", -1},
      {"%stextures/%s-ck.%s", -1},
      {NULL}
    }},
    {chex, GLDT_FLAT, {
      {"%sflats/chex/%s.%s", -1},
      {"%stextures/chex/flat-%s.%s", -1},
      {"%sflats/%s.%s", -1},
      {"%stextures/flat-%s.%s", -1},
      {NULL}
    }},
    {chex, GLDT_PATCH, {
      {"%spatches/chex/%s.%s", -1},
      {"%spatches/%s.%s", -1},
      {NULL}
    }},

    {none, GLDT_UNREGISTERED, {
      {NULL}
    }},
  };

  static char *hiresdir = NULL;

  char texname[9];
  char *texname_p = NULL;
  int i;

  hires_path_item_t *checklist = NULL;
  GLTexType useType = gltexture->textype;

  dboolean supported = (gl_texture_external_hires && 
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
  M_Strlwr(texname);

  if (!hiresdir)
  {
    hiresdir = malloc(PATH_MAX);
    if (strlen(gl_texture_hires_dir) > 0)
    {
      strncpy(hiresdir, gl_texture_hires_dir, PATH_MAX - 1);
    }
    else
    {
      strncpy(hiresdir, I_DoomExeDir(), PATH_MAX - 1);
    }
    // guarantee null delimiter
    hiresdir[PATH_MAX - 1] = 0;

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
      doom_snprintf(checkName, sizeof(checkName), checklist->path, hiresdir, "", "");
      if (!access(checkName, F_OK))
        checklist->exists = 1;
      else
        checklist->exists = 0;
    }
    
    if (checklist->exists == 1) //dir exists
    {
      static const char * extensions[] =
      {"png", "jpg", "tga", "pcx", "gif", "bmp", NULL};
      const char ** extp;

      if (GLEXT_glCompressedTexImage2DARB && dds_path[0] == '\0')
      {
        doom_snprintf(checkName, sizeof(checkName), checklist->path, hiresdir, texname, "dds");
        if (!access(checkName, F_OK))
        {
          strcpy(dds_path, checkName);
        }
      }
      
      for (extp = extensions; *extp; extp++)
      {
        doom_snprintf(checkName, sizeof(checkName), checklist->path, hiresdir, texname, *extp);

        if (!access(checkName, F_OK))
        {
          strcpy(img_path, checkName);
          return true;
        }
      }
    }
  }
  while ((++checklist)->path);

  if (dds_path[0])
    return true;

  return false;
}

static void gld_HiRes_Bind(GLTexture *gltexture, GLuint *glTexID)
{
  switch (gltexture->textype)
  {
  case GLDT_TEXTURE:
    gl_has_hires |= 1;
    break;
  case GLDT_FLAT:
    gl_has_hires |= 2;
    break;
  case GLDT_PATCH:
    gl_has_hires |= 4;
    break;
  }

  if ((gltexture->textype == GLDT_TEXTURE) || (gltexture->textype == GLDT_FLAT))
    gltexture->flags |= GLTEXTURE_MIPMAP;
  else
    gltexture->flags &= ~GLTEXTURE_MIPMAP;

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

void gld_HiRes_ProcessColormap(unsigned char *buffer, int bufSize)
{
  int pos;
  const lighttable_t *colormap;
  const unsigned char *playpal;

  if (!RGB2PAL)
    return;

  playpal = V_GetPlaypal();
  colormap = (fixedcolormap ? fixedcolormap : fullcolormap);

  for (pos = 0; pos < bufSize; pos += 4)
  {
#if 1
    byte color;
    
    if (gl_hires_24bit_colormap)
      color = RGB2PAL[(buffer[pos+0]<<16) + (buffer[pos+1]<<8) + buffer[pos+2]];
    else
      color = RGB2PAL[((buffer[pos+0]>>3)<<10) + ((buffer[pos+1]>>3)<<5) + (buffer[pos+2]>>3)];

    buffer[pos+0] = playpal[colormap[color]*3+0];
    buffer[pos+1] = playpal[colormap[color]*3+1];
    buffer[pos+2] = playpal[colormap[color]*3+2];
#endif

#if 0
    float factor;
    int c, r, g, b, m;
    byte color;

    color = RGB2PAL[(buffer[pos+0]<<16) + (buffer[pos+1]<<8) + buffer[pos+2]];

    factor = 
      0.30f * playpal[color*3+0] + 
      0.59f * playpal[color*3+1] + 
      0.11f * playpal[color*3+2];

    if (fabs(factor) < 0.001f)
      factor = 1;
    else
      factor = (0.3f * buffer[pos+0] + 0.59f * buffer[pos+1] + 0.11f * buffer[pos+2]) / factor;

    r = (int)(playpal[colormap[color]*3+0] * factor);
    g = (int)(playpal[colormap[color]*3+1] * factor);
    b = (int)(playpal[colormap[color]*3+2] * factor);

    m = 255;
    if (r > m)
    {
      m = r;
      factor = 255.0f / (float)playpal[colormap[color]*3+0];
    }
    if (g > m)
    {
      m = g;
      factor = 255.0f / (float)playpal[colormap[color]*3+1];
    }
    if (b > m)
    {
      m = b;
      factor = 255.0f / (float)playpal[colormap[color]*3+2];
    }

    c = (int)(playpal[colormap[color]*3+0] * factor);
    buffer[pos+0] = BETWEEN(0, 255, c);
    c = (int)(playpal[colormap[color]*3+1] * factor);
    buffer[pos+1] = BETWEEN(0, 255, c);
    c = (int)(playpal[colormap[color]*3+2] * factor);
    buffer[pos+2] = BETWEEN(0, 255, c);
#endif

#if 0
    float factor;
    int c;

    color = RGB2PAL[(buffer[pos+0]<<16) + (buffer[pos+1]<<8) + buffer[pos+2]];

    factor = 
      0.30f * playpal[color*3+0] + 
      0.59f * playpal[color*3+1] + 
      0.11f * playpal[color*3+2];

    if (fabs(factor) < 0.001f)
      factor = 1;
    else
      factor = (0.3f * buffer[pos+0] + 0.59f * buffer[pos+1] + 0.11f * buffer[pos+2]) / factor;

    c = (int)(playpal[colormap[color]*3+0] * factor);
    buffer[pos+0] = BETWEEN(0, 255, c);
    c = (int)(playpal[colormap[color]*3+1] * factor);
    buffer[pos+1] = BETWEEN(0, 255, c);
    c = (int)(playpal[colormap[color]*3+2] * factor);
    buffer[pos+2] = BETWEEN(0, 255, c);
#endif
  }
}

int gld_HiRes_BuildTables(void)
{
#define RGB2PAL_NAME "RGB2PAL"
  const int chanel_bits = (gl_hires_24bit_colormap ? 8 : 5);
  const int numcolors_per_chanel = (1 << chanel_bits);
  const int RGB2PAL_size = numcolors_per_chanel * numcolors_per_chanel * numcolors_per_chanel;
  unsigned char* RGB2PAL_fname;
  int lump, size;

  if ((!gl_boom_colormaps) || !(gl_texture_internal_hires || gl_texture_external_hires))
    return false;

  if (RGB2PAL)
    return true;

  if (gl_hires_24bit_colormap)
  {
    lump = W_CheckNumForName(RGB2PAL_NAME);
    if (lump != -1)
    {
      size = W_LumpLength(lump);
      if (size == RGB2PAL_size)
      {
        const byte* RGB2PAL_lump;

        RGB2PAL_lump = W_CacheLumpNum(lump);
        RGB2PAL = malloc(RGB2PAL_size);
        memcpy(RGB2PAL, RGB2PAL_lump, RGB2PAL_size);
        W_UnlockLumpName(RGB2PAL_NAME);
        return true;
      }
    }

    RGB2PAL_fname = I_FindFile(RGB2PAL_NAME".dat", ".dat");
    if (RGB2PAL_fname)
    {
      struct stat RGB24to8_stat;
      memset(&RGB24to8_stat, 0, sizeof(RGB24to8_stat));
      stat(RGB2PAL_fname, &RGB24to8_stat);
      size = 0;
      if (RGB24to8_stat.st_size == RGB2PAL_size)
      {
        I_FileToBuffer(RGB2PAL_fname, &RGB2PAL, &size);
      }
      free(RGB2PAL_fname);

      if (size == RGB2PAL_size)
        return true;
    }
  }

  if (1 || M_CheckParm("-"RGB2PAL_NAME))
  {
    int ok = true;
    FILE *RGB2PAL_fp = NULL;
    char fname[PATH_MAX+1];

    if (gl_hires_24bit_colormap)
    {
      doom_snprintf(fname, sizeof(fname), "%s/"RGB2PAL_NAME".dat", I_DoomExeDir());
      RGB2PAL_fp = fopen(fname, "wb");
      ok = RGB2PAL_fp != NULL;
    }

    if (ok)
    {
      void* NewIntDynArray(int dimCount, int *dims);
      const byte* palette;
      int r, g, b, k, color;
      int **x, **y, **z;
      int dims[2] = {numcolors_per_chanel, 256};

      x = NewIntDynArray(2, dims);
      y = NewIntDynArray(2, dims);
      z = NewIntDynArray(2, dims);

      RGB2PAL = malloc(RGB2PAL_size);
      palette = V_GetPlaypal();

      // create the RGB24to8 lookup table
      gld_ProgressStart();
      gld_ProgressUpdate(NULL, 0, numcolors_per_chanel);
      for (k = 0; k < numcolors_per_chanel; k++)
      {
        int color_p = 0;
        int kk = (gl_hires_24bit_colormap ? k : (k<<3)|(k>>2));
        for (color = 0; color < 256; color++)
        {
          x[k][color] = (kk - palette[color_p++]);
          x[k][color] *= x[k][color];
          y[k][color] = (kk - palette[color_p++]);
          y[k][color] *= y[k][color];
          z[k][color] = (kk - palette[color_p++]);
          z[k][color] *= z[k][color];
        }
      }

      k = 0;
      for (r = 0; r < numcolors_per_chanel; r++)
      {
        gld_ProgressUpdate(NULL, r, numcolors_per_chanel);
        for (g = 0; g < numcolors_per_chanel; g++)
        {
          int xy[256];
          for (color = 0; color < 256; color++)
          {
            xy[color] = x[r][color] + y[g][color];
          }
          for (b = 0; b < numcolors_per_chanel; b++)
          {
            int dist;
            int bestcolor = 0;
            int bestdist = xy[0] + z[b][0];
            #define CHECK_BEST dist = xy[color] + z[b][color];\
              if (dist < bestdist) {bestdist = dist; bestcolor = color;} color++;
            for (color = 0; color < 256;)
            {
              CHECK_BEST;
              CHECK_BEST;
              CHECK_BEST;
              CHECK_BEST;
              CHECK_BEST;
              CHECK_BEST;
              CHECK_BEST;
              CHECK_BEST;
            }
            RGB2PAL[k++] = bestcolor;
          }
        }
      }
      gld_ProgressEnd();

      free(z);
      free(y);
      free(x);

      if (gl_hires_24bit_colormap)
      {
        ok = fwrite(RGB2PAL, RGB2PAL_size, 1, RGB2PAL_fp) == 1;
        return ((fclose(RGB2PAL_fp) == 0) && ok);
      }
      else
      {
        return true;
      }
    }
  }

  gl_boom_colormaps_default = false;
  M_ChangeAllowBoomColormaps();
  return false;
}

void gld_InitHiRes(void)
{
  gld_HiRes_BuildTables();

  gl_has_hires = 0;

  gld_PrecacheGUIPatches();
}

static int gld_HiRes_LoadDDSTexture(GLTexture* gltexture, GLuint* texid, const char* dds_path)
{
  int result = false;
  int tex_width, tex_height;

  if (GLEXT_glCompressedTexImage2DARB)
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

        gld_HiRes_Bind(gltexture, texid);

        if (numMipmaps > 1)
          gltexture->flags |= GLTEXTURE_MIPMAP;
        else
          gltexture->flags &= ~GLTEXTURE_MIPMAP;

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

        gld_SetTexFilters(gltexture);
        
        free(ddsimage->pixels);
        free(ddsimage);

        result = true;
      }
    }
  }

  return result;
}

static int gld_HiRes_LoadFromCache(GLTexture* gltexture, GLuint* texid, const char* img_path)
{
  int result = false;
  int tex_width = 0;
  int tex_height = 0;
  int tex_buffer_size;
  struct stat cache_stat;
  struct stat tex_stat;
  char* cache_filename;
  FILE *cachefp;
  unsigned char *tex_buffer;

  memset(&tex_stat, 0, sizeof(tex_stat));
  stat(img_path, &tex_stat);
  
  cache_filename = malloc(strlen(img_path) + 16);
  sprintf(cache_filename, "%s.cache", img_path);

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
            gld_HiRes_Bind(gltexture, texid);
            gld_BuildTexture(gltexture, tex_buffer, false, tex_width, tex_height);

            result = true;
          }
        }
      }
    }
    fclose(cachefp);
  }

  free(cache_filename);

  return result;
}

static int gld_HiRes_WriteCache(GLTexture* gltexture, GLuint* texid, const char* img_path)
{
  int result = false;
  int w, h;
  unsigned char *buf;
  unsigned char cache_filename[PATH_MAX];
  struct stat tex_stat;
  FILE *cachefp;

  doom_snprintf(cache_filename, sizeof(cache_filename), "%s.cache", img_path);
  if (access(cache_filename, F_OK))
  {
    buf = gld_GetTextureBuffer(*texid, 0, &w, &h);
    if (buf)
    {
      memset(&tex_stat, 0, sizeof(tex_stat));
      stat(img_path, &tex_stat);
      cachefp = fopen(cache_filename, "wb");
      if (cachefp)
      {
        result =
          (fwrite(&w, sizeof(w), 1, cachefp) == 1) &&
          (fwrite(&h, sizeof(h), 1, cachefp) == 1) &&
          (fwrite(&tex_stat.st_mtime, sizeof(tex_stat.st_mtime), 1, cachefp) == 1) &&
          (fwrite(buf, w * h * 4, 1, cachefp) == 1);

         fclose(cachefp);
      }
    }
  }

  if (!result)
  {
    lprintf(LO_WARN, "gld_HiRes_WriteCache: error writing '%s'.\n", cache_filename);
  }

  return result;
}

static int gld_HiRes_LoadFromFile(GLTexture* gltexture, GLuint* texid, const char* img_path)
{
  int result = false;
  SDL_Surface *surf = NULL;
  SDL_Surface *surf_tmp = NULL;

  surf_tmp = IMG_Load(img_path);

  if (!surf_tmp)
  {
    lprintf(LO_WARN, "gld_HiRes_LoadExternal: %s\n", SDL_GetError());
  }
  else
  {
    surf = SDL_ConvertSurface(surf_tmp, &RGBAFormat, 0);
    SDL_FreeSurface(surf_tmp);

    if (surf)
    {
      if (SDL_LockSurface(surf) >= 0)
      {
        if (SmoothEdges(surf->pixels, surf->pitch / 4, surf->h))
          gltexture->flags |= GLTEXTURE_HASHOLES;
        else
          gltexture->flags &= ~GLTEXTURE_HASHOLES;
        SDL_UnlockSurface(surf);
      }
      gld_HiRes_Bind(gltexture, texid);
      result = gld_BuildTexture(gltexture, surf->pixels, true, surf->w, surf->h);

      SDL_FreeSurface(surf);
    }
  }

  return result;
}

int gld_LoadHiresTex(GLTexture *gltexture, int cm)
{
  int result = false;
  GLuint *texid;

  // do we need it?
  if ((gl_texture_external_hires || gl_texture_internal_hires) &&
    !(gltexture->flags & GLTEXTURE_HASNOHIRES))
  {
    // default buffer
    texid = &gltexture->glTexExID[CR_DEFAULT][0][0];

    // do not try to load hires twice
    if ((*texid == 0) || (gltexture->flags & GLTEXTURE_HIRES))
    {
      // try to load in-wad texture
      if (*texid == 0 && gl_texture_internal_hires)
      {
        const char *lumpname = gld_HiRes_GetInternalName(gltexture);

        if (lumpname)
        {
          int lump = (W_CheckNumForName)(lumpname, ns_hires);
          if (lump != -1)
          {
            SDL_RWops *rw_data = SDL_RWFromConstMem(W_CacheLumpNum(lump), W_LumpLength(lump));
            SDL_Surface *surf_tmp = IMG_Load_RW(rw_data, false);
            
            // SDL can't load some TGA with common method
            if (!surf_tmp)
            {
              surf_tmp = IMG_LoadTyped_RW(rw_data, false, "TGA");
            }

            SDL_FreeRW(rw_data);

            if (!surf_tmp)
            {
              lprintf(LO_WARN, "gld_LoadHiresTex: %s\n", SDL_GetError());
            }
            else
            {
              SDL_Surface *surf = SDL_ConvertSurface(surf_tmp, &RGBAFormat, 0);
              SDL_FreeSurface(surf_tmp);

              if (surf)
              {
                if (SDL_LockSurface(surf) >= 0)
                {
                  if (SmoothEdges(surf->pixels, surf->pitch / 4, surf->h))
                    gltexture->flags |= GLTEXTURE_HASHOLES;
                  else
                    gltexture->flags &= ~GLTEXTURE_HASHOLES;
                  SDL_UnlockSurface(surf);
                }
                gld_HiRes_Bind(gltexture, texid);
                gld_BuildTexture(gltexture, surf->pixels, true, surf->w, surf->h);

                SDL_FreeSurface(surf);
              }
            }
          }
        }
      }

      // then external
      if (*texid == 0 && gl_texture_external_hires)
      {
        char img_path[PATH_MAX];
        char dds_path[PATH_MAX];
        if (gld_HiRes_GetExternalName(gltexture, img_path, dds_path))
        {
          if (!gld_HiRes_LoadDDSTexture(gltexture, texid, dds_path))
          {
            if (!gld_HiRes_LoadFromCache(gltexture, texid, img_path))
            {
              if (gld_HiRes_LoadFromFile(gltexture, texid, img_path))
              {
                if ((gltexture->realtexwidth != gltexture->tex_width) ||
                  (gltexture->realtexheight != gltexture->tex_height))
                {
                  gld_HiRes_WriteCache(gltexture, texid, img_path);
                }
              }
            }
          }
        }
      }

      if (*texid)
      {
        gld_GetTextureTexID(gltexture, cm);

        if (last_glTexID == gltexture->texid_p)
        {
          result = true;
        }
        else
        {
          if (texid == gltexture->texid_p)
          {
            glBindTexture(GL_TEXTURE_2D, *gltexture->texid_p);
            result = true;
          }
          else
          {
            //if (gl_boom_colormaps && use_boom_cm &&
            //  !(comp[comp_skymap] && (gltexture->flags&GLTEXTURE_SKY)))
            if (boom_cm && use_boom_cm && gl_boom_colormaps)
            {
              int w, h;
              unsigned char *buf;

              if (*gltexture->texid_p == 0)
              {
                buf = gld_GetTextureBuffer(*texid, 0, &w, &h);
                gld_HiRes_Bind(gltexture, gltexture->texid_p);
                gld_HiRes_ProcessColormap(buf, w * h * 4);
                if (gld_BuildTexture(gltexture, buf, true, w, h))
                {
                  result = true;
                }
              }
              else
              {
                gld_HiRes_Bind(gltexture, gltexture->texid_p);
                result = true;
              }
            }
            else
            {
              *gltexture->texid_p = *texid;
              gld_HiRes_Bind(gltexture, gltexture->texid_p);
              result = true;
            }
          }
        }
      }
    }
  }

  if (result)
  {
    last_glTexID = gltexture->texid_p;
  }
  else
  {
    // there is no corresponding hires
    gltexture->flags |= GLTEXTURE_HASNOHIRES;
  }

  return result;
}

int gld_PrecacheGUIPatches(void)
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
    "M_ACCEL",  "M_AUTO",   "M_BUTT1",  "M_BUTT2",
    "M_CHAT",   "M_COLORS", "M_COMPAT", "M_COMPAT",
    "M_ENEM",   "M_GENERL", "M_HORSEN", "M_KEYBND",
    "M_KEYBND", "M_LOKSEN", "M_MESS",   "M_PALNO",
    "M_PALSEL", "M_SETUP",  "M_STAT",   "M_STAT",
    "M_VBOX",   "M_VERSEN", "M_WEAP",

    NULL
  };

  const char ** patch_p;
  int count, total;

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
      gltexture = gld_RegisterPatch(lump, CR_DEFAULT, false);
      if (gltexture)
      {
        gld_BindPatch(gltexture, CR_DEFAULT);
        if (gltexture && (gltexture->flags & GLTEXTURE_HIRES))
        {
          gld_ProgressUpdate("Loading GUI Patches...", ++count, total);
        }
      }
    }
  }

  gld_ProgressEnd();

  return 0;
}

#endif // HAVE_LIBSDL2_IMAGE
