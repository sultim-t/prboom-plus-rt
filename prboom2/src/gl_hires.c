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

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"
#include "i_system.h"
#include "w_wad.h"

int gl_texture_usehires = -1;
int gl_texture_usehires_default;
int gl_hires_override_pwads;
char *gl_texture_hires_dir = NULL;

static SDL_PixelFormat RGBAFormat;

static void gld_InitHiresTex(void)
{
  static boolean first = true;

  if (first)
  {
    first = false;

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
  }
}

typedef enum {
  STATIC_HIRES_DIR_UNDEFINED,
  STATIC_HIRES_DIR_YES,
  STATIC_HIRES_DIR_NO,
} StaticHiResDirState_t;

static int gld_HiresGetTextureName(GLTexture *gltexture, char *hirespath)
{
  static const char * doom1texpath[]= {
    "%stextures/doom/doom1/%s.%s", "%stextures/doom/doom1/%s-ck.%s",
    "%stextures/doom1/%s.%s",
    "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
    "%stextures/%s.%s", "%stextures/%s-ck.%s",
    NULL
  };

  static const char * doom2texpath[]= {
    "%stextures/doom/doom2/%s.%s", "%stextures/doom/doom2/%s-ck.%s",
    "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
    "%stextures/%s.%s", "%stextures/%s-ck.%s",
    NULL
  };

  static const char * pluttexpath[]= {
    "%stextures/doom/plut/%s.%s", "%stextures/doom/plut/%s-ck.%s",
    "%stextures/doom2-plut/%s.%s",
    "%stextures/doom/doom2-plut/%s.%s", "%stextures/doom/doom2-plut/%s-ck.%s",
    "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
    "%stextures/%s.%s", "%stextures/%s-ck.%s",
    NULL
  };

  static const char * tnttexpath[]= {
    "%stextures/doom/tnt/%s.%s", "%stextures/doom/tnt/%s-ck.%s",
    "%stextures/doom2-tnt/%s.%s",
    "%stextures/doom/doom2-tnt/%s.%s", "%stextures/doom/doom2-tnt/%s-ck.%s",
    "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
    "%stextures/%s.%s", "%stextures/%s-ck.%s",
    NULL
  };

  static const char * doom1flatpath[]= {
    "%sflats/doom/doom1/%s.%s", "%stextures/doom/doom1/flat-%s.%s",
    "%stextures/doom1/flat-%s.%s",
    "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
    "%sflats/%s.%s", "%stextures/flat-%s.%s",
    NULL
  };

  static const char * doom2flatpath[]= {
    "%sflats/doom/doom2/%s.%s", "%stextures/doom/doom2/flat-%s.%s",
    "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
    "%sflats/%s.%s", "%stextures/flat-%s.%s",
    NULL
  };

  static const char * plutflatpath[]= {
    "%sflats/doom/plut/%s.%s", "%stextures/doom/plut/flat-%s.%s",
    "%sflats/doom/doom2-plut/%s.%s", "%stextures/doom/doom2-plut/flat-%s.%s",
    "%stextures/doom2-plut/flat-%s.%s",
    "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
    "%sflats/%s.%s", "%stextures/flat-%s.%s",
    NULL
  };

  static const char * tntflatpath[]= {
    "%sflats/doom/tnt/%s.%s", "%stextures/doom/tnt/flat-%s.%s",
    "%sflats/doom/doom2-tnt/%s.%s", "%stextures/doom/doom2-tnt/flat-%s.%s",
    "%stextures/doom2-tnt/flat-%s.%s",
    "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
    "%sflats/%s.%s", "%stextures/flat-%s.%s",
    NULL
  };

  static StaticHiResDirState_t use_static_hiresdir = STATIC_HIRES_DIR_UNDEFINED;
  static char* static_hiresdir = NULL;

  char texname[9];
  int file_access = 0;

  const char ** checklist;
  BYTE useType = gltexture->textype;

  boolean supported =
    (gltexture->textype == GLDT_TEXTURE) ||
    (gltexture->textype == GLDT_FLAT);

  if (!supported)
    return 0;

  switch (gamemission)
  {
    case doom:
      checklist = useType == GLDT_FLAT ? doom1flatpath : doom1texpath;
      break;
    case doom2:
      checklist = useType == GLDT_FLAT ? doom2flatpath : doom2texpath;
      break;
    case pack_tnt:
      checklist = useType == GLDT_FLAT ? tntflatpath : tnttexpath;
      break;
    case pack_plut:
      checklist = useType == GLDT_FLAT ? plutflatpath : pluttexpath;
      break;
    default:
      return false;
  }

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

      strncpy(texname, texture->name, 8);
      texname[8] = 0;
    }
    break;
  case GLDT_FLAT:
    {
      if (!gl_hires_override_pwads)
      {
        if (lumpinfo[gltexture->index].source != source_iwad)
          return false;
      }

      strncpy(texname, lumpinfo[gltexture->index].name, 8);
      texname[8] = 0;
    }
    break;
  }

  if (use_static_hiresdir == STATIC_HIRES_DIR_UNDEFINED)
  {
    use_static_hiresdir = STATIC_HIRES_DIR_NO;
    if (gl_texture_hires_dir && strlen(gl_texture_hires_dir) > 0)
    {
      const char *p = gl_texture_hires_dir;
      static_hiresdir = malloc(PATH_MAX + 1);
      use_static_hiresdir = STATIC_HIRES_DIR_YES;
#ifdef HAVE_SNPRINTF
      snprintf((char*)static_hiresdir, PATH_MAX, "%s%s", p, HasTrailingSlash(p) ? "" : "\\");
#else
      sprintf((char*)static_hiresdir, "%s%s", p, HasTrailingSlash(p) ? "" : "\\");
#endif
    }
  }

  while (*checklist)
  {
    char checkName[PATH_MAX];
    static const char * extensions[] = { "PNG", "JPG", "TGA", "PCX", "GIF", "BMP", NULL };
    const char ** extp;

    for (extp = extensions; *extp; extp++)
    {
      boolean FileIsPresent;
      boolean UseStaticDir = (use_static_hiresdir == STATIC_HIRES_DIR_YES);

#ifdef HAVE_SNPRINTF
      snprintf(checkName, sizeof(checkName) - 1, *checklist, (UseStaticDir ? static_hiresdir : ""), texname, *extp);
#else
      sprintf(checkName, *checklist, (UseStaticDir ? static_hiresdir : ""), texname, *extp);
#endif

      if (use_static_hiresdir == STATIC_HIRES_DIR_YES)
      {
        FileIsPresent = (access(checkName, 0) == 0);
      }
      else
      {
        FileIsPresent = (I_FindFile2(checkName, *extp) != NULL);
      }

      if (FileIsPresent)
      {
        strcpy(hirespath, checkName);
        return true;
      }
    }
    checklist++;
  }

  return 0;
}

static int gld_LoadHiresItem(GLTexture *gltexture, const char *texture_path, int *glTexID)
{
  int result = false;
  SDL_Surface *surf = NULL;
  SDL_Surface *surf_tmp = NULL;

  gld_InitHiresTex();

  surf_tmp = IMG_Load(texture_path);
  surf = SDL_ConvertSurface(surf_tmp, &RGBAFormat, surf_tmp->flags);
  SDL_FreeSurface(surf_tmp);

  if (surf)
  {
    if (*glTexID == 0)
      glGenTextures(1,glTexID);
    glBindTexture(GL_TEXTURE_2D, *glTexID);

#ifdef USE_GLU_MIPMAP
    if (gltexture->mipmap & use_mipmapping)
    {
      gluBuild2DMipmaps(GL_TEXTURE_2D, gl_tex_format,
        surf->w, surf->h,
        GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_mipmap_filter);
      if (gl_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0);

      result = true;
      goto l_exit;
    }
    else
#endif // USE_GLU_MIPMAP
    {
      int tex_width  = gld_GetTexDimension(surf->w);
      int tex_height = gld_GetTexDimension(surf->h);
#ifdef USE_GLU_IMAGESCALE
      if ((surf->w != tex_width) || (surf->h != tex_height))
      {
        unsigned char *scaledbuffer;

        scaledbuffer = (unsigned char*)Z_Malloc(tex_width * tex_height * 4, PU_STATIC, 0);
        if (scaledbuffer)
        {
          gluScaleImage(GL_RGBA,
            surf->w, surf->h,
            GL_UNSIGNED_BYTE, surf->pixels,
            tex_width, tex_height,
            GL_UNSIGNED_BYTE, scaledbuffer);
          glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
            tex_width, tex_height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, scaledbuffer);
          Z_Free(scaledbuffer);
        }
      }
      else
#endif // USE_GLU_IMAGESCALE
      {
        unsigned char *scaledbuffer = NULL;

        if ((surf->w != tex_width) || (surf->h != tex_height))
        {
          scaledbuffer = (unsigned char*)Z_Malloc(tex_width * tex_height * 4, PU_STATIC, 0);
          memcpy(scaledbuffer, surf->pixels, surf->w * surf->h * 4);
        }
        else
        {
          scaledbuffer = surf->pixels;
        }

        if (gl_paletted_texture) {
          gld_SetTexturePalette(GL_TEXTURE_2D);
          glTexImage2D( GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT,
            tex_width, tex_height,
            0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaledbuffer);
        } else {
          glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
            tex_width, tex_height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, scaledbuffer);
        }

        if ((surf->w != tex_width) || (surf->h != tex_height))
          Z_Free(scaledbuffer);
      }
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);

      result = true;
      goto l_exit;
    }
  }

l_exit:
  if (surf)
    SDL_FreeSurface(surf);

  return result;
}

int gld_LoadHiresTex(GLTexture *gltexture, int *glTexID)
{
  char hirespath[PATH_MAX];

  if (!gl_texture_usehires)
    return 0;

  if (gld_HiresGetTextureName(gltexture, hirespath))
  {
    gld_LoadHiresItem(gltexture, hirespath, glTexID);
    return 1;
  }

  return 0;
}
