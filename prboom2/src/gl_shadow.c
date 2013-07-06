/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright 2006 - 2008 G Jackson, Jaakko Kerônen
 *  Copyright 2009 - Andrey Budko
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
 * DESCRIPTION: Shadow rendering
*    Based on Risen3D implementation which is based on Doomsday v1.7.8.
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gl_opengl.h"
#include "gl_intern.h"
#include "doomstat.h"
#include "p_maputl.h"
#include "w_wad.h"
#include "r_fps.h"
#include "r_bsp.h"
#include "r_sky.h"
#include "r_main.h"
#include "lprintf.h"

int gl_shadows_maxdist;
int gl_shadows_factor;

simple_shadow_params_t simple_shadows =
{
  0, 0,
  -1, 0, 0,
  80, 1000, 0.5f, 0.0044f
};

//===========================================================================
// GL_PrepareLightTexture
//	The dynamic light map is a 64x64 grayscale 8-bit image.
//===========================================================================
void gld_InitShadows(void)
{
  int lump;
  
  simple_shadows.loaded = false;

  simple_shadows.tex_id = -1;
  simple_shadows.width = 0;
  simple_shadows.height = 0;

  simple_shadows.max_radius = 80;
  simple_shadows.max_dist = gl_shadows_maxdist;
  simple_shadows.factor = (float)gl_shadows_factor / 256.0f;
  simple_shadows.bias = 0.0044f;
  
  lump = (W_CheckNumForName)("GLSHADOW", ns_prboom);
  if (lump != -1)
  {
    SDL_PixelFormat fmt;
    SDL_Surface *surf = NULL;
    SDL_Surface *surf_raw;
    surf_raw = SDL_LoadBMP_RW(SDL_RWFromConstMem(W_CacheLumpNum(lump), W_LumpLength(lump)), 1);
    W_UnlockLumpNum(lump);

    fmt = *surf_raw->format;
    fmt.BitsPerPixel = 24;
    fmt.BytesPerPixel = 3;

    surf = SDL_ConvertSurface(surf_raw, &fmt, surf_raw->flags);
    SDL_FreeSurface(surf_raw);
    if (surf)
    {
      glGenTextures(1, &simple_shadows.tex_id);
      glBindTexture(GL_TEXTURE_2D, simple_shadows.tex_id);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, surf->w, surf->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surf->pixels);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      if (gl_ext_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)(1<<gl_texture_filter_anisotropic));

      simple_shadows.loaded = true;
      simple_shadows.width = surf->w;
      simple_shadows.height = surf->h;

      SDL_FreeSurface(surf);
    }
  }

  if (simple_shadows.enable && !simple_shadows.loaded)
  {
    lprintf(LO_INFO, "gld_InitShadows: failed to initialise shadow texture");
  }
}

static void gld_DrawShadow(GLShadow *shadow)
{
  glColor3f(shadow->light, shadow->light, shadow->light);

  glBegin(GL_TRIANGLE_FAN);

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(shadow->x + shadow->radius, shadow->z, shadow->y - shadow->radius);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(shadow->x - shadow->radius, shadow->z, shadow->y - shadow->radius);
  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(shadow->x - shadow->radius, shadow->z, shadow->y + shadow->radius);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(shadow->x + shadow->radius, shadow->z, shadow->y + shadow->radius);

  glEnd();
}

//===========================================================================
// Rend_ProcessThingShadow
// Modified for z-smoothing - R3D
//===========================================================================
void gld_ProcessThingShadow(mobj_t *mo)
{
  int dist;
  float height, moh, halfmoh;
  sector_t *sec = mo->subsector->sector;
  int radius, z;
  fixed_t fz;
  GLShadow shadow;

  if (!simple_shadows.enable || !simple_shadows.loaded)
    return;

  // Should this mobj have a shadow?
  if (mo->flags & (MF_SHADOW|MF_NOBLOCKMAP|MF_NOSECTOR))
    return;

  if (mo->frame & FF_FULLBRIGHT)
    return;
  
  // Don't render mobj shadows on sky floors.
  if (mo->subsector->sector->floorpic == skyflatnum)
    return;

  if (sectorloops[sec->iSectorID].loopcount <= 0)
    return;

  // Is this too far?
  dist = P_AproxDistance((mo->x >> 16) - (viewx >> 16), (mo->y >> 16) - (viewy >> 16));
  if (dist > simple_shadows.max_dist)
    return;

  // Check the height.
  if (sec->heightsec != -1)
    z = sectors[sec->heightsec].floorheight;
  else
    z = sec->floorheight;
  
  // below visible floor
  if (!paused && movement_smooth)
  {
    fz = mo->PrevZ + FixedMul (tic_vars.frac, mo->z - mo->PrevZ);
  }
  else
  {
    fz = mo->z;
  }
  if (fz < z)
    return;

  height = (fz - z) / (float)FRACUNIT;
  moh = mo->height / (float)FRACUNIT;
  if(!moh)
    moh = 1;
  
  // Too high above floor.
  if(height > moh)
    return;

  // Calculate the strength of the shadow.

  shadow.light = simple_shadows.factor * sec->lightlevel / 255.0f;
  
  halfmoh = moh * 0.5f;
  if(height > halfmoh)
    shadow.light *= 1 - (height - halfmoh) / (moh - halfmoh);
  
  // Can't be seen.
  if(shadow.light <= 0)
    return;

  if(shadow.light > 1)
    shadow.light = 1;

  // Calculate the radius of the shadow.
  radius = mo->info->radius >> 16;
  if (radius > mo->patch_width >> 1)
    radius = mo->patch_width >> 1;
  if(radius > simple_shadows.max_radius)
    radius = simple_shadows.max_radius;
  if(!radius)
    return;

  shadow.radius = radius / MAP_COEFF;
  shadow.x = -mo->x / MAP_SCALE;
  shadow.y = mo->y / MAP_SCALE;
  shadow.z = mo->subsector->sector->floorheight / MAP_SCALE + 0.2f / MAP_COEFF;

  gld_AddDrawItem(GLDIT_SHADOW, &shadow);
}

//===========================================================================
// Rend_RenderShadows
//===========================================================================
void gld_RenderShadows(void)
{
  int i;

  if (!simple_shadows.enable || !simple_shadows.loaded || players[displayplayer].fixedcolormap)
    return;

  if (gld_drawinfo.num_items[GLDIT_SHADOW] <= 0)
    return;

  if (!gl_ztrick)
  {
    glDepthRange(simple_shadows.bias, 1);
  }

  gl_EnableFog(false);

  glDepthMask(GL_FALSE);
  glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

  // Apply a modelview shift.
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  // Scale towards the viewpoint to avoid Z-fighting.
  glTranslatef(xCamera, zCamera, yCamera);
  glScalef(0.99f, 0.99f, 0.99f);
  glTranslatef(-xCamera, -zCamera, -yCamera);

  glBindTexture(GL_TEXTURE_2D, simple_shadows.tex_id);
  gld_ResetLastTexture();

  for (i = gld_drawinfo.num_items[GLDIT_SHADOW] - 1; i >= 0; i--)
  {
    gld_DrawShadow(gld_drawinfo.items[GLDIT_SHADOW][i].item.shadow);
  }

  if (!gl_ztrick)
  {
    glDepthRange(0, 1);
  }

  glPopMatrix();
  glDepthMask(GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
