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

typedef struct
{
  int id;
  int count;
  int validcount;
  int ceiling;
  sector_t *sector;
  sector_t **list;
} fakegroup_t;

static int numfakeplanes = 0;
static fakegroup_t *fakeplanes = NULL;
static sector_t **sectors2 = NULL;

static void gld_PrepareSectorSpecialEffects(void);
static void gld_PreprocessFakeSector(int ceiling, sector_t *sector, int groupid);

static void gld_PrepareSectorSpecialEffects(void)
{
  int i, num;

  for (num = 0; num < numsectors; num++)
  {
    // the following is for specialeffects. see r_bsp.c in R_Subsector
    sectors[num].flags=(NO_TOPTEXTURES|NO_BOTTOMTEXTURES);
    for (i=0; i<sectors[num].linecount; i++)
    {
      if ( (sectors[num].lines[i]->sidenum[0]!=NO_INDEX) &&
        (sectors[num].lines[i]->sidenum[1]!=NO_INDEX) )
      {
        if (sides[sectors[num].lines[i]->sidenum[0]].toptexture!=NO_TEXTURE)
          sectors[num].flags &= ~NO_TOPTEXTURES;
        if (sides[sectors[num].lines[i]->sidenum[0]].bottomtexture!=NO_TEXTURE)
          sectors[num].flags &= ~NO_BOTTOMTEXTURES;
        if (sides[sectors[num].lines[i]->sidenum[1]].toptexture!=NO_TEXTURE)
          sectors[num].flags &= ~NO_TOPTEXTURES;
        if (sides[sectors[num].lines[i]->sidenum[1]].bottomtexture!=NO_TEXTURE)
          sectors[num].flags &= ~NO_BOTTOMTEXTURES;
      }
      else
      {
        sectors[num].flags &= ~NO_TOPTEXTURES;
        sectors[num].flags &= ~NO_BOTTOMTEXTURES;
      }
    }
#ifdef _DEBUG
    if (sectors[num].flags & NO_TOPTEXTURES)
      lprintf(LO_INFO,"Sector %i has no toptextures\n",num);
    if (sectors[num].flags & NO_BOTTOMTEXTURES)
      lprintf(LO_INFO,"Sector %i has no bottomtextures\n",num);
#endif
  }
}

//
// Recursive mark of all adjoining sectors with no bottom/top texture
//

static void gld_PreprocessFakeSector(int ceiling, sector_t *sector, int groupid)
{
  int i;

  if (sector->fakegroup[ceiling] != groupid)
  {
    sector->fakegroup[ceiling] = groupid;
    if (groupid >= numfakeplanes)
    {
      fakeplanes = realloc(fakeplanes, (numfakeplanes + 1) * sizeof(fakegroup_t));
      memset(&fakeplanes[numfakeplanes], 0, sizeof(fakegroup_t));
      numfakeplanes++;
    }
    sectors2[fakeplanes[groupid].count++] = sector;
  }

  for (i = 0; i < sector->linecount; i++)
  {
    sector_t *sec = NULL;
    line_t *line = sector->lines[i];

    if (line->frontsector && line->frontsector != sector)
    {
      sec = line->frontsector;
    }
    else
    {
      if (line->backsector && line->backsector != sector)
      {
        sec = line->backsector;
      }
    }

    if (sec && sec->fakegroup[ceiling] == -1 &&
       (sec->flags & (ceiling ? NO_TOPTEXTURES : NO_BOTTOMTEXTURES)))
    {
      gld_PreprocessFakeSector(ceiling, sec, groupid);
    }
  }
}

//
// Split of all sectors into groups
// with adjoining sectors with no bottom/top texture
//

void gld_PreprocessFakeSectors(void)
{
  int i, j, k;
  int groupid;

  // free memory
  if (fakeplanes)
  {
    for (i = 0; i < numfakeplanes; i++)
    {
      fakeplanes[i].count = 0;
      free(fakeplanes[i].list);
      fakeplanes[i].list = NULL;
    }
    numfakeplanes = 0;
    free(fakeplanes);
    fakeplanes = NULL;
  }
  if (sectors2)
  {
    free(sectors2);
  }
  sectors2 = malloc(numsectors * sizeof(sector_t*));

  // reset all groups with fake floors and ceils
  // 0 - floor; 1 - ceil;
  for (i = 0; i < numsectors; i++)
  {
    sectors[i].fakegroup[0] = -1;
    sectors[i].fakegroup[1] = -1;
  }

  // precalculate NO_TOPTEXTURES and NO_BOTTOMTEXTURES flags
  gld_PrepareSectorSpecialEffects();

  groupid = 0;

  do
  {
    for (i = 0; i < numsectors; i++)
    {
      if (!(sectors[i].flags & NO_BOTTOMTEXTURES) && sectors[i].fakegroup[0] == -1)
      {
        gld_PreprocessFakeSector(0, &sectors[i], groupid);
        fakeplanes[groupid].ceiling = 0;
        fakeplanes[groupid].list = malloc(fakeplanes[groupid].count * sizeof(sector_t*));
        for (j = 0, k = 0; k < fakeplanes[groupid].count; k++)
        {
          if (!(sectors2[k]->flags & NO_BOTTOMTEXTURES))
          {
            fakeplanes[groupid].list[j++] = sectors2[k];
          }
        }
        fakeplanes[groupid].count = j;
        groupid++;
        break;
      }
    }
  }
  while (i < numsectors);

  // the same with ceilings
  do
  {
    for (i = 0; i < numsectors; i++)
    {
      if (!(sectors[i].flags & NO_TOPTEXTURES) && sectors[i].fakegroup[1] == -1)
      {
        gld_PreprocessFakeSector(1, &sectors[i], groupid);
        fakeplanes[groupid].ceiling = 1;
        fakeplanes[groupid].list = malloc(fakeplanes[groupid].count * sizeof(sector_t*));
        for (j = 0, k = 0; k < fakeplanes[groupid].count; k++)
        {
          if (!(sectors2[k]->flags & NO_TOPTEXTURES))
          {
            fakeplanes[groupid].list[j++] = sectors2[k];
          }
        }
        fakeplanes[groupid].count = j;
        groupid++;
        break;
      }
    }
  }
  while (i < numsectors);
}

//
// Get highest surounding floorheight for flors and
// lowest surounding ceilingheight for ceilings
//

sector_t* GetBestFake(sector_t *sector, int ceiling, int validcount)
{
  int i;
  int groupid = sector->fakegroup[ceiling];

  if (groupid == -1)
    return NULL;

  if (fakeplanes[groupid].validcount != validcount)
  {
    fakeplanes[groupid].validcount = validcount;
    fakeplanes[groupid].sector = NULL;

    if (fakeplanes[groupid].ceiling)
    {
      fixed_t min_height = INT_MAX;
      for (i = 0; i < fakeplanes[groupid].count; i++)
      {
        if (!(fakeplanes[groupid].list[i]->flags & NO_TOPTEXTURES) &&
          fakeplanes[groupid].list[i]->ceilingheight < min_height)
        {
          min_height = fakeplanes[groupid].list[i]->ceilingheight;
          fakeplanes[groupid].sector = fakeplanes[groupid].list[i];
        }
      }
    }
    else
    {
      fixed_t max_height = INT_MIN;
      for (i = 0; i < fakeplanes[groupid].count; i++)
      {
        if (!(fakeplanes[groupid].list[i]->flags & NO_BOTTOMTEXTURES) &&
          fakeplanes[groupid].list[i]->floorheight > max_height)
        {
          max_height = fakeplanes[groupid].list[i]->floorheight;
          fakeplanes[groupid].sector = fakeplanes[groupid].list[i];
        }
      }
    }
  }

  if (fakeplanes[groupid].sector)
  {
    if (fakeplanes[groupid].ceiling)
    {
      if (sector->ceilingheight < fakeplanes[groupid].sector->ceilingheight)
      {
        return sector;
      }
    }
    else
    {
      if (sector->floorheight > fakeplanes[groupid].sector->floorheight)
      {
        return sector;
      }
    }
  }

  return fakeplanes[groupid].sector;
}
