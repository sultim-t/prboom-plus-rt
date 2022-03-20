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

#include "e6y.h"
#include "m_misc.h"

#define RT_MAX_MAPS 512

typedef struct
{
  int episode;
  int map;
  int sectorcount;
  float *sectorweights;
} rt_map_metainfo_t;


static int rt_mission = -1;
static rt_map_metainfo_t rt_maps[RT_MAX_MAPS];
static int rt_maps_count = 0;


static const char *GetMissionFile(int mission)
{
  switch (mission)
  {
    case doom: return RG_RESOURCES_FOLDER"map_metainfo_doom1.txt";
    default: return NULL;
  }
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

      rt_maps_count++;
      if (rt_maps_count >= (int)RG_ARRAY_SIZE(rt_maps))
      {
        break;
      }

      if (strcmp(curr_line, "@END") == 0)
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

      cur_em->sectorweights = calloc(cur_em->sectorcount, sizeof(float));
      sector_iter = 0;
    }
    else if (cur_em != NULL)
    {
      cur_em->sectorweights[sector_iter] = strtof(curr_line, NULL);
      sector_iter++;
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

  const char *filepath = GetMissionFile(rt_mission);

  if (filepath == NULL)
  {
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

    assert(rt_maps[i].sectorcount > 0 && rt_maps[i].sectorweights != NULL);

    for (int s = 0; s < rt_maps[i].sectorcount; s++)
    {
      float w = rt_maps[i].sectorweights[s];
      w = BETWEEN(0, 1, w);

      fprintf(fp, "%f\n", w);
    }
  }

  fprintf(fp, "@END\n");

  fclose(fp);
}


void RT_MapMetaInfo_Init(int mission)
{
  rt_mission = mission;
  rt_maps_count = 0;
  

  const char *filepath = GetMissionFile(mission);

  if (filepath == NULL)
  {
    lprintf(LO_ERROR, "Unknown map pack. Maps won't have additional light sources.\n", filepath);
    return;
  }

  byte *buffer = NULL;
  int length = M_ReadFile(filepath, &buffer);

  if (length <= 0 || buffer == NULL)
  {
    Z_Free(buffer);

    if (length == 0)
    {
      lprintf(LO_WARN, "%s is empty. Maps won't have additional light sources.\n", filepath);
    }
    else
    {
      lprintf(LO_ERROR, "%s wasn't found. Maps won't have additional light sources.\n", filepath);
    }
    return;
  }

  Parse(buffer, length);

  Z_Free(buffer);
}


static rt_map_metainfo_t *GetMapMetaInfo(int mission, int episode, int map)
{
  if (rt_mission != mission)
  {
    return NULL;
  }

  for (int i = 0; i < rt_maps_count; i++)
  {
    if (rt_maps[i].episode == episode && rt_maps[i].map == map)
    {
      return &rt_maps[i];

    }
  }

  return NULL;
}


#include "doomstat.h"


float RT_GetSectorLightLevelWeight(int sectornum)
{
  const rt_map_metainfo_t *mp = GetMapMetaInfo(gamemission, gameepisode, gamemap);

  if (mp == NULL)
  {
    return 0;
  }

  if (mp->sectorweights == NULL || sectornum >= mp->sectorcount)
  {
    assert(0);
    return 0;
  }

  return mp->sectorweights[sectornum];
}


void RT_MapMetaInfo_AddDelta(float delta)
{
  rt_map_metainfo_t *mp = GetMapMetaInfo(gamemission, gameepisode, gamemap);

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
    mp->sectorweights = calloc(numsectors, sizeof(float));
  }

  int sectornum = RT_GetSectorNum_Fixed(viewx, viewy);

  if (mp->sectorweights == NULL || sectornum >= mp->sectorcount)
  {
    assert(0);
    return;
  }
  
  mp->sectorweights[sectornum] = BETWEEN(0, 1, mp->sectorweights[sectornum] + delta);
}
