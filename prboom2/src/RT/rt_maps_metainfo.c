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

#include "rt_main.h"

#include "doomstat.h"
#include "e6y.h"
#include "m_argv.h"
#include "m_misc.h"

#define RT_MAX_MAPS 512
#define RT_MAX_WEIGHT 3.0f

typedef struct
{
  float weight;
  uint8_t r, g, b;
} rt_sector_metainfo_t;

typedef struct
{
  int episode;
  int map;
  int sectorcount;
  rt_sector_metainfo_t *sectors;
} rt_map_metainfo_t;


static rt_map_metainfo_t rt_maps[RT_MAX_MAPS];
static int rt_maps_count = 0;


static const char *GetMapMetaInfoFile()
{
  int i = M_CheckParm("-iwadrt");

  if (i && (++i < myargc))
  {
    return myargv[i];
  }

  // use default one if Doom 1993
  if (gamemission == doom)
  {
    return RG_RESOURCES_FOLDER"map_metainfo_doom1.txt";
  }

  return NULL;
}


static void Parse(const char *text, int length)
{
  int text_iter = 0;
  char curr_line[256];

  rt_map_metainfo_t *cur_em = NULL;
  int sector_iter = 0;

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
          lprintf(LO_WARN, "Map metainfo file contains line with >=255 characters (char id: %d).\n", text_iter);
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

    if (curr_line[0] == '@')
    {
      cur_em = &rt_maps[rt_maps_count];
      memset(cur_em, 0, sizeof(*cur_em));

      if (strcmp(curr_line, "@END") == 0)
      {
        break;
      }

      rt_maps_count++;
      if (rt_maps_count >= (int)RG_ARRAY_SIZE(rt_maps))
      {
        break;
      }

      int c = sscanf(&curr_line[1], "%d %d %d", 
                     &cur_em->episode,
                     &cur_em->map,
                     &cur_em->sectorcount);
      if (c != 3)
      {
        I_Error("Map metainfo file wrong format");
      }

      // check prev sector count == line count in a file for that map
      if (rt_maps_count >= 2)
      {
        assert(rt_maps[rt_maps_count - 2].sectorcount == sector_iter);
      }

      cur_em->sectors = calloc(cur_em->sectorcount, sizeof(rt_sector_metainfo_t));
      sector_iter = 0;
    }
    else if (cur_em != NULL)
    {
      rt_sector_metainfo_t *dst = &cur_em->sectors[sector_iter];
      sector_iter++;

      float weight = 0;
      char str_hexcolor[8] = { 0 };
      int c = sscanf(curr_line, "%f %6s", &weight, str_hexcolor);

      if (c == 1)
      {
        dst->weight = strtof(curr_line, NULL);
        dst->r = 255;
        dst->g = 255;
        dst->b = 255;
      }
      else if (c == 2)
      {
        const char red[] = { str_hexcolor[0], str_hexcolor[1], '\0' };
        uint32_t ir = strtoul(red, NULL, 16);

        const char green[] = { str_hexcolor[2], str_hexcolor[3], '\0' };
        uint32_t ig = strtoul(green, NULL, 16);

        const char blue[] = { str_hexcolor[4], str_hexcolor[5], '\0' };
        uint32_t ib = strtoul(blue, NULL, 16);

        dst->weight = strtof(curr_line, NULL);
        dst->r = (uint8_t)BETWEEN(0, 255, ir);
        dst->g = (uint8_t)BETWEEN(0, 255, ig);
        dst->b = (uint8_t)BETWEEN(0, 255, ib);
      }
      else
      {
        I_Error("Map metainfo: wrong format for sectors");
      }
    }

    // ---
  }
}


void RT_MapMetaInfo_WriteToFile(void)
{
  // "@1 1 234"
  // "0"
  // "2"
  // "4"
  // "0"
  // "@END"

  const char *filepath = GetMapMetaInfoFile();

  if (filepath == NULL)
  {
    I_Warning("Specify -iwadrt to save map meta info.\n", filepath);
    return;
  }

  FILE *fp = fopen(filepath, "wb");

  if (fp == NULL)
  {
    return;
  }

  for (int i = 0; i < rt_maps_count; i++)
  {
    fprintf(fp, "@%d %d %d\n", rt_maps[i].episode, rt_maps[i].map, rt_maps[i].sectorcount);

    assert(rt_maps[i].sectorcount > 0 && rt_maps[i].sectors != NULL);

    for (int s = 0; s < rt_maps[i].sectorcount; s++)
    {
      const rt_sector_metainfo_t *src = &rt_maps[i].sectors[s];

      float lightweight = BETWEEN(0, RT_MAX_WEIGHT, src->weight);
      int r = src->r;
      int g = src->g;
      int b = src->b;

      fprintf(fp, "%f %02x%02x%02x\n", lightweight, r, g, b);
    }
  }

  fprintf(fp, "@END\n");

  fclose(fp);
}


void RT_MapMetaInfo_Init(int mission)
{
  rt_maps_count = 0;
  

  const char *filepath = GetMapMetaInfoFile();

  if (filepath == NULL)
  {
    I_Warning("Maps won't have additional light sources: can't find a map meta info file. Specify \"-iwadrt <file>\".\n");
    return;
  }

  byte *buffer = NULL;
  int length = M_ReadFile(filepath, &buffer);

  if (length < 0)
  {
    I_Warning("Maps won't have additional light sources: can't find the file \"%s\".\n", filepath);
    return;
  }

  if (length == 0 || buffer == NULL)
  {
    Z_Free(buffer);

    if (length == 0)
    {
      I_Warning("%s is empty. Maps won't have additional light sources.\n", filepath);
    }
    else
    {
      I_Warning("%s wasn't found. Maps won't have additional light sources.\n", filepath);
    }
    return;
  }

  Parse(buffer, length);

  Z_Free(buffer);
}


static rt_map_metainfo_t *GetMapMetaInfo(int episode, int map)
{
  for (int i = 0; i < rt_maps_count; i++)
  {
    if (rt_maps[i].episode == episode && rt_maps[i].map == map)
    {
      return &rt_maps[i];
    }
  }

  return NULL;
}


dboolean RT_GetSectorLightLevelWeight(int                sectornum,
                                      float*             out_weight,
                                      RgColor4DPacked32* out_color)
{
  const rt_map_metainfo_t *mp = GetMapMetaInfo(gameepisode, gamemap);

  if (mp == NULL)
  {
    return false;
  }

  if (mp->sectors == NULL || sectornum >= mp->sectorcount)
  {
    assert(0);
    return false;
  }

  const rt_sector_metainfo_t *src = &mp->sectors[sectornum];

  if (src->weight <= 0.0f)
  {
    return false;
  }

  *out_weight = src->weight;
  *out_color  = rgUtilPackColorByte4D(src->r, src->g, src->b, 255);

  return true;
}


extern subsector_t* R_PointInSubsector(fixed_t x, fixed_t y);

static int RT_GetSectorNum_Fixed(fixed_t x, fixed_t y)
{
  return R_PointInSubsector(x, y)->sector->iSectorID;
}


void RT_MapMetaInfo_AddDelta(float deltaweight, int deltared, int deltagreen, int deltablue)
{
  rt_map_metainfo_t *mp = GetMapMetaInfo(gameepisode, gamemap);

  if (mp == NULL)
  {
    if (rt_maps_count >= (int)RG_ARRAY_SIZE(rt_maps))
    {
      return;
    }

    mp = &rt_maps[rt_maps_count];
    memset(mp, 0, sizeof(*mp));
    rt_maps_count++;

    mp->episode = gameepisode;
    mp->map = gamemap;
    mp->sectorcount = numsectors;
    mp->sectors = calloc(numsectors, sizeof(rt_sector_metainfo_t));
  }

  int sectornum = RT_GetSectorNum_Fixed(viewx, viewy);

  if (mp->sectors == NULL || sectornum >= mp->sectorcount)
  {
    assert(0);
    return;
  }


  rt_sector_metainfo_t *dst = &mp->sectors[sectornum];


  if (dst->weight <= 1.01f)
  {
    dst->weight = BETWEEN(0, RT_MAX_WEIGHT, dst->weight + deltaweight);
  }
  else
  {
    // if >1 then jump to max value
    dst->weight = deltaweight > 0 ? RT_MAX_WEIGHT : 1.0f;
  }

  dst->r = (uint8_t)BETWEEN(0, 255, dst->r + deltared);
  dst->g = (uint8_t)BETWEEN(0, 255, dst->g + deltagreen);
  dst->b = (uint8_t)BETWEEN(0, 255, dst->b + deltablue);
}
