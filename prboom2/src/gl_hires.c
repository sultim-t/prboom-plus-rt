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
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "doomstat.h"
#include "v_video.h"
#include "gl_intern.h"
#include "i_system.h"
#include "w_wad.h"
#include "lprintf.h"

int gl_texture_usehires = -1;
int gl_texture_usehires_default;
int gl_patch_usehires = -1;
int gl_patch_usehires_default;
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

typedef struct {
  GameMission_t gamemission;
  GLTexType textype;
  const char * texpath[16];
} hires_path_t;

static int gld_HiresGetTextureName(GLTexture *gltexture, char *hirespath)
{
  static const hires_path_t hires_paths[] = {
    {doom, GLDT_TEXTURE, {
      "%stextures/doom/doom1/%s.%s", "%stextures/doom/doom1/%s-ck.%s",
      "%stextures/doom1/%s.%s",
      "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
      "%stextures/%s.%s", "%stextures/%s-ck.%s",
      NULL
    }},
    {doom, GLDT_FLAT, {
      "%sflats/doom/doom1/%s.%s", "%stextures/doom/doom1/flat-%s.%s",
      "%stextures/doom1/flat-%s.%s",
      "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
      "%sflats/%s.%s", "%stextures/flat-%s.%s",
      NULL
    }},
    {doom, GLDT_PATCH, {
      "%spatches/doom/doom1/%s.%s",
      "%spatches/doom1-ultimate/%s.%s",
      "%spatches/doom/%s.%s",
      "%spatches/%s.%s",
      NULL
    }},

    {doom2, GLDT_TEXTURE, {
      "%stextures/doom/doom2/%s.%s", "%stextures/doom/doom2/%s-ck.%s",
      "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
      "%stextures/%s.%s", "%stextures/%s-ck.%s",
      NULL
    }},
    {doom2, GLDT_FLAT, {
      "%sflats/doom/doom2/%s.%s", "%stextures/doom/doom2/flat-%s.%s",
      "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
      "%sflats/%s.%s", "%stextures/flat-%s.%s",
      NULL
    }},
    {doom2, GLDT_PATCH, {
      "%spatches/doom/doom2/%s.%s",
      "%spatches/doom2/%s.%s",
      "%spatches/doom/%s.%s",
      "%spatches/%s.%s",
      NULL
    }},

    {pack_tnt, GLDT_TEXTURE, {
      "%stextures/doom/tnt/%s.%s", "%stextures/doom/tnt/%s-ck.%s",
      "%stextures/doom2-tnt/%s.%s",
      "%stextures/doom/doom2-tnt/%s.%s", "%stextures/doom/doom2-tnt/%s-ck.%s",
      "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
      "%stextures/%s.%s", "%stextures/%s-ck.%s",
      NULL
    }},
    {pack_tnt, GLDT_FLAT, {
      "%sflats/doom/tnt/%s.%s", "%stextures/doom/tnt/flat-%s.%s",
      "%sflats/doom/doom2-tnt/%s.%s", "%stextures/doom/doom2-tnt/flat-%s.%s",
      "%stextures/doom2-tnt/flat-%s.%s",
      "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
      "%sflats/%s.%s", "%stextures/flat-%s.%s",
      NULL
    }},
    {pack_tnt, GLDT_PATCH, {
      "%spatches/doom/tnt/%s.%s",
      "%spatches/doom2-tnt/%s.%s",
      "%spatches/tnt/%s.%s",
      "%spatches/doom/%s.%s",
      "%spatches/%s.%s",
      NULL
    }},

    {pack_plut, GLDT_TEXTURE, {
      "%stextures/doom/plut/%s.%s", "%stextures/doom/plut/%s-ck.%s",
      "%stextures/doom2-plut/%s.%s",
      "%stextures/doom/doom2-plut/%s.%s", "%stextures/doom/doom2-plut/%s-ck.%s",
      "%stextures/doom/%s.%s", "%stextures/doom/%s-ck.%s",
      "%stextures/%s.%s", "%stextures/%s-ck.%s",
      NULL
    }},
    {pack_plut, GLDT_FLAT, {
      "%sflats/doom/plut/%s.%s", "%stextures/doom/plut/flat-%s.%s",
      "%sflats/doom/doom2-plut/%s.%s", "%stextures/doom/doom2-plut/flat-%s.%s",
      "%stextures/doom2-plut/flat-%s.%s",
      "%sflats/doom/%s.%s", "%stextures/doom/flat-%s.%s",
      "%sflats/%s.%s", "%stextures/flat-%s.%s",
      NULL
    }},
    {pack_plut, GLDT_PATCH, {
      "%spatches/doom/plut/%s.%s",
      "%spatches/doom2-plut/%s.%s",
      "%spatches/plutonia/%s.%s",
      "%spatches/doom/%s.%s",
      "%spatches/%s.%s",
      NULL
    }},

    {none, GLDT_UNREGISTERED, {
      NULL
    }},
  };

  static char *hiresdir = NULL;

  char texname[9];
  char *texname_p;
  int i;

  const char ** checklist = NULL;
  BYTE useType = gltexture->textype;

  boolean supported =
    (gl_texture_usehires && ((useType == GLDT_TEXTURE) || (useType == GLDT_FLAT))) ||
    (gl_patch_usehires && (useType == GLDT_PATCH));

  if (!supported)
    return 0;

  i = 0;
  while (hires_paths[i].gamemission != none)
  {
    if (gamemission == hires_paths[i].gamemission && 
      gltexture->textype == hires_paths[i].textype)
    {
      checklist = (const char **) &hires_paths[i].texpath;
      break;
    }
    i++;
  }
  if (!checklist)
    return 0;

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

  strncpy(texname, texname_p, 8);
  texname[8] = 0;

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
      strcat(hiresdir, "\\");
  }

  while (*checklist)
  {
    char checkName[PATH_MAX + 1];
    static const char * extensions[] = { "PNG", "JPG", "TGA", "PCX", "GIF", "BMP", NULL };
    const char ** extp;

    for (extp = extensions; *extp; extp++)
    {
#ifdef HAVE_SNPRINTF
      snprintf(checkName, sizeof(checkName), *checklist, hiresdir, texname, *extp);
#else
      sprintf(checkName, *checklist, hiresdir, texname, *extp);
#endif

      if (!access(checkName, F_OK))
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

  if (!surf_tmp)
  {
    lprintf(LO_ERROR, "gld_LoadHiresItem: ");
    lprintf(LO_ERROR, SDL_GetError());
    lprintf(LO_ERROR, "\n");
    return false;
  }

  surf = SDL_ConvertSurface(surf_tmp, &RGBAFormat, surf_tmp->flags);
  SDL_FreeSurface(surf_tmp);

  if (gltexture->textype == GLDT_PATCH)
  {
    gltexture->scalexfac = 1.0f;
    gltexture->scaleyfac = 1.0f;
  }

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
        scaledbuffer = Z_Malloc(tex_width * tex_height * 4, PU_STATIC, 0);

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
          if (surf->w == tex_width)
          {
            scaledbuffer = (unsigned char*)Z_Malloc(tex_width * tex_height * 4, PU_STATIC, 0);
            memcpy(scaledbuffer, surf->pixels, surf->w * surf->h * 4);
          }
          else
          {
            int y;
            scaledbuffer = Z_Calloc(1, tex_width * tex_height * 4, PU_STATIC, 0);
            for (y = 0; y < surf->h; y++)          
            {
              memcpy(scaledbuffer + y * tex_width * 4,
                ((unsigned char*)surf->pixels) + y * surf->w * 4, surf->w * 4);
            }
          }
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

  if (!gl_texture_usehires && !gl_patch_usehires)
    return 0;

  if (gld_HiresGetTextureName(gltexture, hirespath))
  {
    return gld_LoadHiresItem(gltexture, hirespath, glTexID);
  }

  return 0;
}

int gld_PrecachePatches(void)
{
  static const char * staticpatches[] = {
    "INTERPIC", "TITLEPIC",

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
    /*"M_ABOUT",  "M_ACCEL",  "M_AUTO",   "M_BUTT1",
    "M_BUTT2",  "M_CHAT",   "M_COLORS", "M_COMPAT",
    "M_COMPAT", "M_DEMOS",  "M_ENEM",   "M_FEAT",
    "M_GENERL", "M_HORSEN", "M_HUD",    "M_KEYBND",
    "M_KEYBND", "M_LOKSEN", "M_MESS",   "M_MOUSE",
    "M_MULTI",  "M_PALNO",  "M_PALSEL", "M_SETUP",
    "M_SLIDEL", "M_SLIDEM", "M_SLIDEO", "M_SLIDER",
    "M_SOUND",  "M_STAT",   "M_STAT",   "M_VBOX",
    "M_VERSEN", "M_VIDEO",  "M_WAD",    "M_WEAP",*/

    NULL
  };

  const char ** patch_p;

  if (!gl_patch_usehires)
    return 0;

  for (patch_p = staticpatches; *patch_p; patch_p++)
  {
    int lump = W_CheckNumForName(*patch_p);
    if (lump > 0)
    {
      GLTexture *gltexture;

      staticlumps[lump] = true;
      gltexture = gld_RegisterPatch(lump, CR_DEFAULT);
      gld_BindPatch(gltexture, CR_DEFAULT);
    }
  }

  return 0;
}
