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
#include "r_main.h"


#define STRETCHSKY 0

#define SKYDETAIL 16

#define SKYROWS 4
#define SKYCOLUMNS (4 * SKYDETAIL)

#define SKY_TRIANGLE_FAN 1
#define SKY_TRIANGLE_STRIP 2

typedef struct
{
  int mode;
  int vertexcount;
  int vertexindex;
  int use_texture;
} RTSkyLoopDef;


typedef struct
{
  int                rows, columns;
  int                loopcount;
  RTSkyLoopDef*      loops;
  RgPrimitiveVertex* data;
  uint32_t           vertex_count;
} RTSkyVBO;


static int texw;
static float yMult, yAdd;
static dboolean foglayer;
static float delta = 0.0f;


static void SkyVertex(
    RgPrimitiveVertex* vbo, int r, int c, dboolean yflip, dboolean sky_gldwf_skyflip)
{
  static fixed_t scale = 10000 << FRACBITS;
  static angle_t maxSideAngle = ANG180 / 3;

  angle_t topAngle = (angle_t)(c / (float)SKYCOLUMNS * ANGLE_MAX);
  angle_t sideAngle = maxSideAngle * (SKYROWS - r) / SKYROWS;
  fixed_t height = finesine[sideAngle >> ANGLETOFINESHIFT];
  fixed_t realRadius = FixedMul(scale, finecosine[sideAngle >> ANGLETOFINESHIFT]);
  fixed_t x = FixedMul(realRadius, finecosine[topAngle >> ANGLETOFINESHIFT]);
  fixed_t y = (!yflip) ? FixedMul(scale, height) : FixedMul(scale, height) * -1;
  fixed_t z = FixedMul(realRadius, finesine[topAngle >> ANGLETOFINESHIFT]);

  float timesRepeat = (short)(4 * (256.0f / texw));
  if (timesRepeat == 0.0f)
  {
    timesRepeat = 1.0f;
  }

  if (!foglayer)
  {
    vbo->color = rgUtilPackColorByte4D(255, 255, 255, r == 0 ? 0 : 255);

    // And the texture coordinates.
    if (!yflip)	// Flipped Y is for the lower hemisphere.
    {
      vbo->texCoord[0] = (-timesRepeat * c / (float)SKYCOLUMNS);
      vbo->texCoord[1]= (r / (float)SKYROWS) * 1.f * yMult + yAdd;
    }
    else
    {
      vbo->texCoord[0] = (-timesRepeat * c / (float)SKYCOLUMNS);
      vbo->texCoord[1] = ((SKYROWS - r) / (float)SKYROWS) * 1.f * yMult + yAdd;

      // RT: why need such fix to flip uv on lower hemisphere
      vbo->texCoord[1] *= -1;
    }

    if (sky_gldwf_skyflip)
    {
      vbo->texCoord[0] *= -1;
    }
  }

  if (r != 4)
  {
    y += FRACUNIT * 300;
  }

  // And finally the vertex.
  vbo->position[0] = -(float)x / (float)MAP_SCALE;	// Doom mirrors the sky vertically!
  vbo->position[1] = (float)y / (float)MAP_SCALE + delta;
  vbo->position[2] = (float)z / (float)MAP_SCALE;
}


static void BuildSky(RTSkyVBO *vbo, const rt_texture_t *sky_texture, float sky_y_offset, dboolean sky_gldwf_skyflip)
{
  int row_count = SKYROWS;
  int col_count = SKYCOLUMNS;

  int texh, c, r;
  int vertex_count = 2 * row_count * (col_count * 2 + 2) + col_count * 2;

  if ((vbo->columns != col_count) || (vbo->rows != row_count))
  {
    free(vbo->loops);
    free(vbo->data);
    memset(vbo, 0, sizeof(vbo[0]));
  }

  if (!vbo->data)
  {
    memset(vbo, 0, sizeof(vbo[0]));
    vbo->loops = malloc((row_count * 2 + 2) * sizeof(vbo->loops[0]));
    // create vertex array
    vbo->data = malloc(vertex_count * sizeof(vbo->data[0]));
    vbo->vertex_count = vertex_count;
  }

  vbo->columns = col_count;
  vbo->rows = row_count;

  texh = sky_texture->height;
  if (texh > 190 && STRETCHSKY)
    texh = 190;
  texw = sky_texture->width;

  RgPrimitiveVertex *vertex_p = &vbo->data[0];
  vbo->loopcount = 0;
  for (int yflip = 0; yflip < 2; yflip++)
  {
    vbo->loops[vbo->loopcount].mode = SKY_TRIANGLE_FAN;
    vbo->loops[vbo->loopcount].vertexindex = vertex_p - &vbo->data[0];
    vbo->loops[vbo->loopcount].vertexcount = col_count;
    vbo->loops[vbo->loopcount].use_texture = false;
    vbo->loopcount++;

    byte skyColor[3] = { 255,255,255 };

    yAdd = sky_y_offset / texh;
    yMult = (texh <= 180 ? 1.0f : 180.0f / texh);
    if (yflip == 0)
    {
      // skyColor = &sky->CeilingSkyColor[vbo_idx];
    }
    else
    {
      // skyColor = &sky->FloorSkyColor[vbo_idx];
      if (texh <= 180) yMult = 1.0f; else yAdd += 180.0f / texh;
    }

    delta = 0.0f;
    foglayer = true;
    for (c = 0; c < col_count; c++)
    {
      SkyVertex(vertex_p, 1, c, yflip, sky_gldwf_skyflip);
      vertex_p->color = rgUtilPackColorByte4D(skyColor[0], skyColor[1], skyColor[2], 255);
      vertex_p->texCoord[0] = vertex_p->texCoord[1] = 0; // RT: parts that are not covered just use (0,0) of the sky texture
      vertex_p++;
    }
    foglayer = false;

    delta = (yflip ? 5.0f : -5.0f) / MAP_COEFF;

    for (r = 0; r < row_count; r++)
    {
      vbo->loops[vbo->loopcount].mode = SKY_TRIANGLE_STRIP;
      vbo->loops[vbo->loopcount].vertexindex = vertex_p - &vbo->data[0];
      vbo->loops[vbo->loopcount].vertexcount = 2 * col_count + 2;
      vbo->loops[vbo->loopcount].use_texture = true;
      vbo->loopcount++;

      for (c = 0; c <= col_count; c++)
      {
        SkyVertex(vertex_p++, r + (yflip ? 1 : 0), (c ? c : 0), yflip, sky_gldwf_skyflip);
        SkyVertex(vertex_p++, r + (yflip ? 0 : 1), (c ? c : 0), yflip, sky_gldwf_skyflip);
      }
    }
  }
}


static int GetTriangleCount(int mode, int vertex_count)
{
  assert(mode == SKY_TRIANGLE_STRIP || mode == SKY_TRIANGLE_FAN);
  return i_max(0, vertex_count - 2);
}


static int GetIndexCount(const RTSkyVBO *vbo)
{
  int index_count = 0;
  for (int i = 0; i < vbo->loopcount; i++)
  {
    RTSkyLoopDef *loop = &vbo->loops[i];
    index_count += 3 * GetTriangleCount(loop->mode, loop->vertexcount);
  }
  return index_count;
}


void RT_AddSkyDome(void)
{
  if (rtmain.sky.texture == NULL)
  {
    return;
  }

  static RTSkyVBO v_vbo = { 0 };
  static const rt_texture_t *prev_texture = NULL;
  static uint32_t *p_indices = NULL;
  static int index_iter = 0;

  if (rtmain.sky.texture != prev_texture)
  {
    free(v_vbo.loops);
    free(v_vbo.data);
    free(p_indices);
    index_iter = 0;

    memset(&v_vbo, 0, sizeof(v_vbo));
    BuildSky(&v_vbo, rtmain.sky.texture, rtmain.sky.y_offset, rtmain.sky.gldwf_skyflip);
    p_indices = malloc(GetIndexCount(&v_vbo) * sizeof(uint32_t));

    prev_texture = rtmain.sky.texture;


    for (int i = 0; i < v_vbo.loopcount; i++)
    {
      RTSkyLoopDef *loop = &v_vbo.loops[i];
    
      //if (!loop->use_texture)
      //  continue;
    
      switch (loop->mode)
      {
        case SKY_TRIANGLE_STRIP:
        {
          for (int t = 0; t < GetTriangleCount(loop->mode, loop->vertexcount); t++)
          {
            p_indices[index_iter] = loop->vertexindex + t;               index_iter++;
            p_indices[index_iter] = loop->vertexindex + t + (1 + t % 2); index_iter++;
            p_indices[index_iter] = loop->vertexindex + t + (2 - t % 2); index_iter++;
          }
          break;
        }
        case SKY_TRIANGLE_FAN:
        {
          for (int t = 0; t < GetTriangleCount(loop->mode, loop->vertexcount); t++)
          {
            p_indices[index_iter] = loop->vertexindex + t + 1;           index_iter++;
            p_indices[index_iter] = loop->vertexindex + t + 2;           index_iter++;
            p_indices[index_iter] = loop->vertexindex + 0;               index_iter++;
          }
          break;
        }
        default: assert(0); break;
      }
    }
  }

  // TODO: RT: rotate sky by: sky.x_offset
  // glRotatef(-180.0f + rtmain.sky.x_offset, 0.f, 1.f, 0.f);
  float scale[3] = { -2,2,2 };
  float translate[3] = { 0, -1250.0f / MAP_COEFF, 0 };

  if (!STRETCHSKY)
  {
    int texh = rtmain.sky.texture->height;

    if (texh <= 180)
    {
      RG_VEC3_MULTIPLY(scale, 1.0f, (float)texh / 230.0f, 1.0f);
      RG_VEC3_MULTIPLY_V(translate, scale);
    }
    else
    {
      if (texh > 190)
      {
        RG_VEC3_MULTIPLY(scale, 1.0f, 230.0f / 240.0f, 1.0f);
        RG_VEC3_MULTIPLY_V(translate, scale);
      }
    }
  }

  RgMeshPrimitiveInfo prim = {
      .sType                = RG_STRUCTURE_TYPE_MESH_PRIMITIVE_INFO,
      .pNext                = NULL,
      .flags                = RG_MESH_PRIMITIVE_SKY,
      .pPrimitiveNameInMesh = NULL,
      .primitiveIndexInMesh = 0,
      .pVertices            = v_vbo.data,
      .vertexCount          = v_vbo.vertex_count,
      .pIndices             = p_indices,
      .indexCount           = index_iter,
      .pTextureName         = rtmain.sky.texture->name,
      .color                = RG_PACKED_COLOR_WHITE,
      .emissive             = 0.0f,
  };

  RgMeshInfo info = {
      .sType          = RG_STRUCTURE_TYPE_MESH_INFO,
      .pNext          = NULL,
      .uniqueObjectID = RT_GetUniqueID_Sky(),
      .pMeshName      = NULL,
      .transform      = { {
          { scale[0], 0, 0, translate[0] },
          { 0, scale[1], 0, translate[1] },
          { 0, 0, scale[2], translate[2] },
      } },
      .isExportable   = false,
      .animationName  = NULL,
      .animationTime  = 0.0f,
  };

  RgResult r = rgUploadMeshPrimitive(rtmain.instance, &info, &prim);
  RG_CHECK(r);
}
