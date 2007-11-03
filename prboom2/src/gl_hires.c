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
#include "e6y.h"

#ifndef HAVE_STRLWR
char* strlwr(char* str);
#endif

int gl_texture_usehires = -1;
int gl_texture_usehires_default;
int gl_patch_usehires = -1;
int gl_patch_usehires_default;
int gl_hires_override_pwads;
char *gl_texture_hires_dir = NULL;

#ifdef HAVE_LIBSDL_IMAGE

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
  GLTexType useType = gltexture->textype;

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

  while (*checklist)
  {
    char checkName[PATH_MAX + 1];
    static const char * extensions[] = { "png", "jpg", "tga", "pcx", "gif", "bmp", "jpeg",
                                         NULL };
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

  return false;
}

static int gld_LoadHiresItem(GLTexture *gltexture, const char *texture_path, int *glTexID, int cm)
{
  int result = false;

  SDL_Surface *surf = NULL;
  SDL_Surface *surf_tmp = NULL;
  int tex_width, tex_height, tex_buffer_size;
  unsigned char *tex_buffer = NULL;
  
  char cache_filename[PATH_MAX];
  FILE *cachefp = NULL;
  int cache_write = false;
  int cache_read = false;
  int cache_read_ok = false;

  struct stat tex_stat;

  memset(&tex_stat, 0, sizeof(tex_stat));
  stat(texture_path, &tex_stat);

  gltexture->flags |= GLTEXTURE_HIRES;

  if (gltexture->textype == GLDT_PATCH)
  {
    gltexture->scalexfac = 1.0f;
    gltexture->scaleyfac = 1.0f;
  }

  if (*glTexID == 0)
    glGenTextures(1,glTexID);
  glBindTexture(GL_TEXTURE_2D, *glTexID);

  strcpy(cache_filename, texture_path);
  strcat(cache_filename, ".cache");

  if (!access(cache_filename, F_OK))
  {
    cache_read = !access(cache_filename, R_OK);
  }
  else
  {
    cache_write = true;
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

          tex_buffer = Z_Malloc(tex_buffer_size, PU_STATIC, 0);
          if (tex_buffer)
          {
            if (fread(tex_buffer, tex_buffer_size, 1, cachefp) == 1)
            {
          
              glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
                tex_width, tex_height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, tex_buffer);

              cache_read_ok = true;
            }
            Z_Free(tex_buffer);
          }
        }
      }
      fclose(cachefp);
    }
  }
  
  if (!cache_read_ok)
  {
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

    if (!surf)
      return false;
    
    tex_width  = gld_GetTexDimension(surf->w);
    tex_height = gld_GetTexDimension(surf->h);
    tex_buffer_size = tex_width * tex_height * 4;

#ifdef USE_GLU_MIPMAP
    if (gltexture->mipmap & use_mipmapping)
    {
      gluBuild2DMipmaps(GL_TEXTURE_2D,
        surf->format->BytesPerPixel,
        surf->w, surf->h,
        imageformats[surf->format->BytesPerPixel],
        GL_UNSIGNED_BYTE, surf->pixels);
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
#ifdef USE_GLU_IMAGESCALE
      if ((surf->w != tex_width) || (surf->h != tex_height))
      {
        tex_buffer = Z_Malloc(tex_buffer_size, PU_STATIC, 0);
        if (!tex_buffer)
        {
          result = false;
          goto l_exit;
        }

        gluScaleImage(GL_RGBA,
          surf->w, surf->h,
          GL_UNSIGNED_BYTE, surf->pixels,
          tex_width, tex_height,
          GL_UNSIGNED_BYTE, tex_buffer);
    
        if (cache_write || !cache_read_ok)
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
        }
    
        glTexImage2D( GL_TEXTURE_2D, 0, gl_tex_format,
          tex_width, tex_height,
          0, GL_RGBA, GL_UNSIGNED_BYTE, tex_buffer);
    
        Z_Free(tex_buffer);
      }
      else
#endif // USE_GLU_IMAGESCALE
      {
        if ((surf->w != tex_width) || (surf->h != tex_height))
        {
          if (surf->w == tex_width)
          {
            tex_buffer = (unsigned char*)Z_Malloc(tex_buffer_size, PU_STATIC, 0);
            memcpy(tex_buffer, surf->pixels, surf->w * surf->h * 4);
          }
          else
          {
            int y;
            tex_buffer = Z_Calloc(1, tex_buffer_size, PU_STATIC, 0);
            for (y = 0; y < surf->h; y++)          
            {
              memcpy(tex_buffer + y * tex_width * 4,
                ((unsigned char*)surf->pixels) + y * surf->w * 4, surf->w * 4);
            }
          }
        }
        else
        {
          tex_buffer = surf->pixels;
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
    
        if ((surf->w != tex_width) || (surf->h != tex_height))
          Z_Free(tex_buffer);
      }
    }
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_tex_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_tex_filter);

  result = true;
  goto l_exit;

l_exit:
  if (surf)
    SDL_FreeSurface(surf);

  return result;
}

#endif //HAVE_LIBSDL_IMAGE

int gld_LoadHiresTex(GLTexture *gltexture, int *glTexID, int cm)
{
#ifndef HAVE_LIBSDL_IMAGE
  return false;
#else
  char hirespath[PATH_MAX];

  if (!gl_texture_usehires && !gl_patch_usehires)
    return false;

  if (gld_HiresGetTextureName(gltexture, hirespath))
  {
    return gld_LoadHiresItem(gltexture, hirespath, glTexID, cm);
  }

  return false;
#endif
}

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

  if (!gl_texture_usehires && !gl_patch_usehires)
    return;

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

static int gld_PrecachePatch(const char *name, int cm)
{
  int lump = W_CheckNumForName(name);
  if (lump > 0)
  {
    GLTexture *gltexture;
    
    lumpinfo[lump].flags |= LUMP_STATIC;
    gltexture = gld_RegisterPatch(lump, cm);
    gld_BindPatch(gltexture, cm);
    return 1;
  }
  return 0;
}

static void gld_Mark_CM2RGB_Lump(const char *name)
{
  int lump = W_CheckNumForName(name);
  if (lump > 0)
  {
    lumpinfo[lump].flags |= LUMP_CM2RGB;
  }
}

#endif //HAVE_LIBSDL_IMAGE

int gld_PrecachePatches(void)
{
#ifndef HAVE_LIBSDL_IMAGE
  return 0;
#else
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

  if (!gl_patch_usehires)
    return 0;

  gld_ProgressStart();

  count = 0; total = 0;
  for (patch_p = staticpatches; *patch_p; patch_p++)
    total++;

  for (patch_p = staticpatches; *patch_p; patch_p++)
  {
    gld_PrecachePatch(*patch_p, CR_DEFAULT);
    gld_ProgressUpdate("Loading Patches...", ++count, total);
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
#endif
}
