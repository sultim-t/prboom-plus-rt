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
 * DESCRIPTION: Cool automap things
 *
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "SDL.h"
#ifdef HAVE_LIBSDL2_IMAGE
#include "SDL_image.h"
#endif

#include "gl_opengl.h"
#include "gl_intern.h"
#include "w_wad.h"
#include "m_misc.h"
#include "am_map.h"
#include "lprintf.h"

am_icon_t am_icons[am_icon_count + 1] = 
{
  {-1, "M_SHADOW"},

  {-1, "M_ARROW"},
  {-1, "M_NORMAL"},
  {-1, "M_HEALTH"},
  {-1, "M_ARMOUR"},
  {-1, "M_AMMO"},
  {-1, "M_KEY"},
  {-1, "M_POWER"},
  {-1, "M_WEAP"},

  {-1, "M_ARROW"},
  {-1, "M_ARROW"},
  {-1, "M_ARROW"},
  {-1, "M_MARK"},
  {-1, "M_NORMAL"},

  {-1, NULL},
};

typedef struct map_nice_thing_s
{
  vbo_xy_uv_rgba_t v[4];
} PACKEDATTR map_nice_thing_t;

static array_t map_things[am_icon_count];

void gld_InitMapPics(void)
{
  int i, lump;

  i = 0;
  while (am_icons[i].name)
  {
    lump = (W_CheckNumForName)(am_icons[i].name, ns_prboom);
    am_icons[i].lumpnum = lump;
    if (lump != -1)
    {
      SDL_Surface *surf = NULL;
#ifdef HAVE_LIBSDL2_IMAGE
      SDL_Surface *surf_raw;

      surf_raw = IMG_Load_RW(SDL_RWFromConstMem(W_CacheLumpNum(lump), W_LumpLength(lump)), true);

      surf = SDL_ConvertSurface(surf_raw, &RGBAFormat, 0);
      SDL_FreeSurface(surf_raw);
#endif

      W_UnlockLumpNum(lump);

      if (surf)
      {
        glGenTextures(1, &am_icons[i].tex_id);
        glBindTexture(GL_TEXTURE_2D, am_icons[i].tex_id);

        if (gl_arb_texture_non_power_of_two)
        {
          glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
          glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_format, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
        }
        else
        {
          gluBuild2DMipmaps(GL_TEXTURE_2D, gl_tex_format, surf->w, surf->h, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);//tex_filter[MIP_PATCH].min_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//tex_filter[MIP_PATCH].mag_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        SDL_FreeSurface(surf);
      }
    }

    i++;
  }
}

void gld_AddNiceThing(int type, float x, float y, float radius, float angle,
                     unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  map_nice_thing_t *thing = M_ArrayGetNewItem(&map_things[type], sizeof(thing[0]));

  float sina_r = (float)sin(angle) * radius;
  float cosa_r = (float)cos(angle) * radius;

#define MAP_NICE_THING_INIT(index, _x, _y, _u, _v) \
  { \
    thing->v[index].x = _x; \
    thing->v[index].y = _y; \
    thing->v[index].u = _u; \
    thing->v[index].v = _v; \
    thing->v[index].r = r; \
    thing->v[index].g = g; \
    thing->v[index].b = b; \
    thing->v[index].a = a; \
  } \

  MAP_NICE_THING_INIT(0, x + sina_r + cosa_r, y - cosa_r + sina_r, 1.0f, 0.0f);
  MAP_NICE_THING_INIT(1, x + sina_r - cosa_r, y - cosa_r - sina_r, 0.0f, 0.0f);
  MAP_NICE_THING_INIT(2, x - sina_r - cosa_r, y + cosa_r - sina_r, 0.0f, 1.0f);
  MAP_NICE_THING_INIT(3, x - sina_r + cosa_r, y + cosa_r + sina_r, 1.0f, 1.0f);

#undef MAP_NICE_THING_INIT
}

void gld_DrawNiceThings(int fx, int fy, int fw, int fh)
{
  int i;

  glScissor(fx, SCREENHEIGHT - (fy + fh), fw, fh);
  glEnable(GL_SCISSOR_TEST);

  glDisable(GL_ALPHA_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gld_EnableTexture2D(GL_TEXTURE0_ARB, true);

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  // activate vertex array, texture coord array and color arrays
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
#endif

  for (i = 0; i < am_icon_count; i++)
  {
    array_t *things = &map_things[i];

    if (things->count == 0)
      continue;

    glBindTexture(GL_TEXTURE_2D, am_icons[i].tex_id);

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
    {
      map_nice_thing_t *thing = &((map_nice_thing_t*)things->data)[0];

      // activate and specify pointers to arrays
      glVertexPointer(2, GL_FLOAT, sizeof(thing->v[0]), &thing->v[0].x);
      glTexCoordPointer(2, GL_FLOAT, sizeof(thing->v[0]), &thing->v[0].u);
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(thing->v[0]), &thing->v[0].r);

      glDrawArrays(GL_QUADS, 0, things->count * 4);
    }
#else
    for (i = 0; i < things->count; i++)
    {
      map_nice_thing_t *thing = &((map_nice_thing_t*)things->data)[i];

      glColor4ubv(&thing->v[0].r);

      glBegin(GL_TRIANGLE_FAN);
      {
        glTexCoord2f(thing->v[0].u, thing->v[0].v);
        glVertex2f(thing->v[0].x, thing->v[0].y);
        glTexCoord2f(thing->v[1].u, thing->v[1].v);
        glVertex2f(thing->v[1].x, thing->v[1].y);
        glTexCoord2f(thing->v[2].u, thing->v[2].v);
        glVertex2f(thing->v[2].x, thing->v[2].y);
        glTexCoord2f(thing->v[3].u, thing->v[3].v);
        glVertex2f(thing->v[3].x, thing->v[3].y);
      }
      glEnd();
    }
#endif
  }

#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  // deactivate vertex array, texture coord array and color arrays
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
#endif

  gld_ResetLastTexture();
  glDisable(GL_SCISSOR_TEST);
}

void gld_ClearNiceThings(void)
{
  int type;

  for (type = 0; type < am_icon_count; type++)
  {
    M_ArrayClear(&map_things[type]);
  }
}

void gld_DrawMapLines(void)
{
#if defined(USE_VERTEX_ARRAYS) || defined(USE_VBO)
  if (map_lines.count > 0)
  {
    map_point_t *point = (map_point_t*)map_lines.data;

    gld_EnableTexture2D(GL_TEXTURE0_ARB, false);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(2, GL_FLOAT, sizeof(point[0]), &point->x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(point[0]), &point->r);

    glDrawArrays(GL_LINES, 0, map_lines.count * 2);

    gld_EnableTexture2D(GL_TEXTURE0_ARB, true);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
  }
#endif
}
