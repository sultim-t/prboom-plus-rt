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
#include "p_maputl.h"
#include "w_wad.h"
#include "r_bsp.h"

#define SHADOWBIAS 0.0044f

int gl_shadows;

int gl_shadow_max_radius = 80;
int gl_shadow_max_dist = 1000;
float gl_shadow_factor = 0.5f;

static int shadow_id = 0;
static int use_shadows = false;

//===========================================================================
// GL_PrepareLightTexture
//	The dynamic light map is a 64x64 grayscale 8-bit image.
//===========================================================================
void gld_InitShadows(void)
{
  int lump;
  
  use_shadows = false;
  
  lump = (W_CheckNumForName)("SHADOW", ns_prboom);
  if (lump != -1)
  {
    int i;
    unsigned char *buffer, *pixel;
    const unsigned char *data = W_CacheLumpNum(lump);

    glGenTextures(1, &shadow_id);
    glBindTexture(GL_TEXTURE_2D, shadow_id);

    // No mipmapping or resizing is needed, upload directly.
    buffer = calloc(64 * 64, 4);
    if (buffer)
    {
      pixel = buffer;
      for(i = 0; i < 64 * 64; i++, pixel += 3)
      {
        pixel[0] = pixel[1] = pixel[2] = data[i];
      }

      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      if (gl_ext_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)(1<<gl_texture_filter_anisotropic));

      use_shadows = true;

      free(buffer);
    }
    W_UnlockLumpNum(lump);
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
  GLShadow shadow;

  if (!use_shadows)
    return;

  if (mo->flags & (MF_SHADOW|MF_NOBLOCKMAP|MF_NOSECTOR))
    return;

  if (sectorloops[sec->iSectorID].loopcount <= 0)
    return;

  // Is this too far?
  dist = P_AproxDistance((mo->x >> 16) - (viewx >> 16), (mo->y >> 16) - (viewy >> 16));
  if (dist > gl_shadow_max_dist)
    return;

  // Check the height.
  if (sec->heightsec != -1)
    z = sectors[sec->heightsec].floorheight;
  else
    z = sec->floorheight;
  
  // below visible floor
  if (mo->z < z)
    return;

  height = (mo->z - z) / (float)FRACUNIT;
  moh = mo->height / (float)FRACUNIT;
  if(!moh)
    moh = 1;
  
  // Too high above floor.
  if(height > moh)
    return;

  // Calculate the strength of the shadow.

  shadow.light = gl_shadow_factor * sec->lightlevel / 255.0f;
  
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
  if(radius > gl_shadow_max_radius)
    radius = gl_shadow_max_radius;
  if(!radius)
    return;

  shadow.radius = radius / MAP_COEFF;
  shadow.x = -mo->x / MAP_SCALE;
  shadow.y = mo->y / MAP_SCALE;
  shadow.z = mo->subsector->sector->floorheight / MAP_SCALE + 0.5f / MAP_COEFF;

  gld_AddDrawItem(GLDIT_SHADOW, &shadow);
}

//===========================================================================
// Rend_RenderShadows
//===========================================================================
void gld_RenderShadows(void)
{
  int i;

  if (!gl_shadows || !use_shadows)
    return;

  if (gld_drawinfo.num_items[GLDIT_SHADOW] <= 0)
    return;

  glDepthRange(SHADOWBIAS, 1);
  glDepthMask(GL_FALSE);
  glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

  glBindTexture(GL_TEXTURE_2D, shadow_id);
  gld_ResetLastTexture();

  for (i = gld_drawinfo.num_items[GLDIT_SHADOW] - 1; i >= 0; i--)
  {
    gld_DrawShadow(gld_drawinfo.items[GLDIT_SHADOW][i].item.shadow);
  }

  glDepthRange(0, 1);
  glDepthMask(GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
