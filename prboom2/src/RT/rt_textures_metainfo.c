/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  Copyright (C) 2022 by
 *  Sultim Tsyrendashiev
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

#include <stdio.h>

#include "rt_main.h"
#include "rt_textures.h"

#include "e6y.h"
#include "lprintf.h"
#include "m_misc.h"
#include "z_zone.h"


#define RT_TEXTURE_METAINFO_FILE RG_RESOURCES_FOLDER"textures_metainfo.txt"


#define member_size(type, member) sizeof(((type *)0)->member)


#define MAX_METAINFO_COUNT 1024
static rt_texture_metainfo_t rt_metainfo[MAX_METAINFO_COUNT];
static int rt_metainfo_count = 0;


void RT_TextureMetaInfo_Init(void)
{
  rt_metainfo_count = 0;


  byte *buffer = NULL;
  int length = M_ReadFile(RT_TEXTURE_METAINFO_FILE, &buffer);

  if (length <= 0 || buffer == NULL)
  {
    Z_Free(buffer);

    if (length == 0)
    {
      lprintf(LO_WARN, RT_TEXTURE_METAINFO_FILE" is empty. Textures won't have RT effects.\n");
    }
    else
    {
      I_Warning("Couldn't read file "RT_TEXTURE_METAINFO_FILE". Textures won't have RT effects.");
    }
    return;
  }


  enum state_t
  {
    STATE_NONE,
    STATE_WATER,
    STATE_RASTERIZED_WITH_LIGHT,
    STATE_VERTICAL_CONELIGHT,
    STATE_EMISSIVE,
    STATE_MONOCHROME_FOR_COLORMAPS,
  };
  enum state_t state = STATE_NONE;


  const char *text = (const char *)buffer;
  int text_iter = 0;
  char curr_line[256];

  while (true)
  {
    {
      dboolean found_end = text_iter >= length || text[text_iter] == '\0';
      if (found_end)
      {
        break;
      }
    }

    {
      int i = 0;

      while (text_iter < length && text[text_iter] != '\r' && text[text_iter] != '\n' && text[text_iter] != '\0')
      {
        if (i >= (int)sizeof(curr_line) - 1)
        {
          lprintf(LO_WARN, RT_TEXTURE_METAINFO_FILE" contains line with >=255 characters (char id: %d).\n", text_iter);
          break;
        }

        curr_line[i] = text[text_iter];
        i++;
        text_iter++;
      }

      curr_line[i] = '\0';
      text_iter++;
    }

    if (curr_line[0] == '\0')
    {
      continue;
    }

    // ---

    if (strcmp(curr_line, "@WATER") == 0)
    {
      state = STATE_WATER;
      continue;
    }
    else if (strcmp(curr_line, "@RASTERIZED_WITH_LIGHT") == 0)
    {
      state = STATE_RASTERIZED_WITH_LIGHT;
      continue;
    }
    else if (strcmp(curr_line, "@VERTICAL_CONELIGHT") == 0)
    {
      state = STATE_VERTICAL_CONELIGHT;
      continue;
    }
    else if (strcmp(curr_line, "@EMISSIVE") == 0 ||
             strcmp(curr_line, "@EMISSIVE_WITHOUT_INDIRECT_ILLUMINATION") == 0)
    {
      state = STATE_EMISSIVE;
      continue;
    }
    else if (strcmp(curr_line, "@MONOCHROME_FOR_COLORMAPS") == 0)
    {
      state = STATE_MONOCHROME_FOR_COLORMAPS;
      continue;
    }


    dboolean           valid = false;
    char               name[256];
    RgColor4DPacked32  light_color      = rgUtilPackColorByte4D(0, 0, 0, 0);
    rt_texture_flags_t additional_flags = 0;
    float              geom_emission    = 0;
    float              falloff_mult     = 1.0f;


    switch (state)
    {
      case STATE_WATER:
      case STATE_MONOCHROME_FOR_COLORMAPS:
      {
        int c = sscanf(curr_line, "%s", name);
        if (c == 1)
        {
          valid = true;
        }
        break;
      }
      case STATE_RASTERIZED_WITH_LIGHT:
      case STATE_VERTICAL_CONELIGHT:
      {
        char str_hexcolor[8];
        int c = sscanf(curr_line, "%s %6s %f", name, str_hexcolor, &falloff_mult);
        if (c >= 2)
        {
          const char red[] = { str_hexcolor[0], str_hexcolor[1], '\0' };
          uint32_t ir = strtoul(red, NULL, 16);

          const char green[] = { str_hexcolor[2], str_hexcolor[3], '\0' };
          uint32_t ig = strtoul(green, NULL, 16);

          const char blue[] = { str_hexcolor[4], str_hexcolor[5], '\0' };
          uint32_t ib = strtoul(blue, NULL, 16);

          light_color = rgUtilPackColorByte4D((uint8_t)ir, (uint8_t)ig, (uint8_t)ib, 255);

          if (c == 3)
          {
            falloff_mult = BETWEEN(0.01f, 10.0f, falloff_mult);
          }

          valid = true;
        }
        break;
      }
      case STATE_EMISSIVE:
      {
        int c = sscanf(curr_line, "%s %f", name, &geom_emission);
        if (c == 2)
        {
          geom_emission = i_max(geom_emission, 0.0f);
          valid = true;
        }
        break;
      }
      default:
      {
        state = STATE_NONE;
        break;
      }
    }

    if (valid && name[0] == '#')
    {
      // comment
      continue;
    }

    if (!valid)
    {
      if (state == STATE_NONE)
      {
        lprintf(LO_WARN, RT_TEXTURE_METAINFO_FILE": ignoring %s: state (that starts with @) was not specified\n", curr_line);
      }
      else
      {
        lprintf(LO_WARN, RT_TEXTURE_METAINFO_FILE": ignoring %s\n", curr_line);
      }
      continue;
    }

    if (strnlen(name, sizeof(name)) + 1 > (int)member_size(rt_texture_t, name))
    {
      name[sizeof(name) - 1] = '\0';
      lprintf(LO_WARN, RT_TEXTURE_METAINFO_FILE": texture name \'%s\' has more than %d characters. Ignoring.\n", name, (int)member_size(rt_texture_t, name));
      continue;
    }

    {
      rt_texture_metainfo_t *dst = &rt_metainfo[rt_metainfo_count];
      memset(dst, 0, sizeof(*dst));

      memcpy(dst->name, name, sizeof(dst->name));
      dst->name[sizeof(dst->name) - 1] = '\0';

      switch (state)
      {
      case STATE_WATER: 
        dst->additional_flags = RT_TEXTURE_FLAG_IS_WATER_BIT;
        break;

      case STATE_RASTERIZED_WITH_LIGHT:
        dst->additional_flags = RT_TEXTURE_FLAG_WITH_LIGHTSOURCE_BIT | RT_TEXTURE_FLAG_IS_EMISSIVE_BIT;
        dst->light_color = light_color;
        dst->falloff_multiplier = falloff_mult;
        dst->geom_emission = falloff_mult;
        break;

      case STATE_VERTICAL_CONELIGHT:
        dst->additional_flags = RT_TEXTURE_FLAG_WITH_VERTICAL_CONELIGHT_BIT;
        dst->light_color = light_color;
        dst->falloff_multiplier = falloff_mult;
        break;

      case STATE_EMISSIVE:
        dst->additional_flags = RT_TEXTURE_FLAG_IS_EMISSIVE_BIT;
        dst->geom_emission = geom_emission;
        break;

      case STATE_MONOCHROME_FOR_COLORMAPS:
        dst->additional_flags = RT_TEXTURE_FLAG_MONOCHROME_FOR_COLORMAPS_BIT;
        break;

      default: 
        continue;
      }
    }

    // ---

    rt_metainfo_count++;

    if (rt_metainfo_count >= (int)RG_ARRAY_SIZE(rt_metainfo))
    {
      break;
    }
  }

  Z_Free(buffer);
}


const rt_texture_metainfo_t *RT_TextureMetaInfo_Find(const char *name)
{
  if (name == NULL || name[0] == '\0')
  {
    return NULL;
  }

  for (int i = 0; i < rt_metainfo_count; i++)
  {
    if (strnicmp(rt_metainfo[i].name, name, sizeof(rt_metainfo[i].name)) == 0)
    {
      return &rt_metainfo[i];
    }
  }

  return NULL;
}
