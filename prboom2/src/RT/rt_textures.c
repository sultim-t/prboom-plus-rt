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

#include "rt_textures.h"

#include "rt_main.h"
#include "r_patch.h"
#include "v_video.h"
#include "w_wad.h"


static struct
{
  rt_texture_t *all_TXTR;
  rt_texture_t *all_PFS;
  rt_texture_t *all_PFS_STATIC;
}
rttextures = {0 };


static rt_texture_t *RT_Texture_AllocEntry_T(int id, rt_texture_t *all_T)
{
  assert(id >= 0);

  rt_texture_t *td = &all_T[id];

  if (td->exists)
  {
    assert_always("Trying to alloc texture with ID that already exists");
    return NULL;
  }

  memset(td, 0, sizeof(*td));
  td->exists = true;

  return td;

  /*
  for (int i = 0; i < ; i++)
  {
    rt_texture_t *td = &all_T[i];

    if (!td->exists)
    {
      td->exists = true;
      return td;
    }
  }

  assert_always("Not enough texture entries");
  return NULL;
  */
}


void RT_Texture_Init(void)
{
  assert(rttextures.all_TXTR == NULL && rttextures.all_PFS == NULL && rttextures.all_PFS_STATIC == NULL);

  // TODO: should be one global array instead of 3?
  rttextures.all_TXTR       = malloc(numtextures  * sizeof(rt_texture_t));
  rttextures.all_PFS        = malloc(numlumps     * sizeof(rt_texture_t));
  rttextures.all_PFS_STATIC = malloc(numlumps     * sizeof(rt_texture_t));

  memset(rttextures.all_TXTR,       0, numtextures  * sizeof(rt_texture_t));
  memset(rttextures.all_PFS,        0, numlumps     * sizeof(rt_texture_t));
  memset(rttextures.all_PFS_STATIC, 0, numlumps     * sizeof(rt_texture_t));

  RT_TextureMetaInfo_Init();
}


static void FreeRgHandles(const rt_texture_t *all, int count)
{
  if (all == NULL || count == 0)
  {
    return;
  }

  for (int i = 0; i < count; i++)
  {
    const rt_texture_t *td = &all[i];

    if (td->exists && td->rg_handle != RG_NO_MATERIAL)
    {
      RgResult r = rgDestroyMaterial(rtmain.instance, td->rg_handle);
      RG_CHECK(r);
    }
  }
}


void RT_Texture_Clean_WithoutStatic(void)
{
  FreeRgHandles(rttextures.all_TXTR, numtextures);
  FreeRgHandles(rttextures.all_PFS, numlumps);

  memset(rttextures.all_TXTR,       0, numtextures * sizeof(rt_texture_t));
  memset(rttextures.all_PFS,        0, numlumps    * sizeof(rt_texture_t));
}


void RT_Texture_Clean_Static(void)
{
  FreeRgHandles(rttextures.all_PFS_STATIC, numlumps);
  memset(rttextures.all_PFS_STATIC, 0, numlumps * sizeof(rt_texture_t));
}


void RT_Texture_Destroy(void)
{
  assert(rttextures.all_TXTR != NULL && rttextures.all_PFS != NULL && rttextures.all_PFS_STATIC != NULL);
  RT_Texture_Clean_WithoutStatic();
  RT_Texture_Clean_Static();

  free(rttextures.all_TXTR);
  free(rttextures.all_PFS);
  free(rttextures.all_PFS_STATIC);

  memset(&rttextures, 0, sizeof(rttextures));
}


void RT_Texture_PrecacheTextures(void)
{}

enum rt_texture_directory_type { RT_DIR_TYPE_GFX, RT_DIR_TYPE_FLAT };
#define FLAT_FORMAT "flat/%s"
#define GFX_FORMAT "gfx/%s"
#define RT_TEXTURE_PATH_FORMAT(directory_type) ((directory_type) == RT_DIR_TYPE_FLAT ? FLAT_FORMAT : GFX_FORMAT)

static void SetNameFromLump(rt_texture_t *dst, const int lump, enum rt_texture_directory_type directory_type)
{
  assert(directory_type == RT_DIR_TYPE_GFX || directory_type == RT_DIR_TYPE_FLAT);
  const lumpinfo_t *l = W_GetLumpInfoByNum(lump);

  char safe_name[sizeof(l->name)];
  memcpy(safe_name, l->name, sizeof(l->name));
  safe_name[sizeof(l->name) - 1] = '\0';

  // format+name+'\0' must be less
  assert((strlen(RT_TEXTURE_PATH_FORMAT(directory_type)) - 2) + strlen(safe_name) + 1 <= sizeof(dst->name));

  snprintf(dst->name, sizeof(dst->name), RT_TEXTURE_PATH_FORMAT(directory_type), safe_name);
}

static void SetNameFromTexture(rt_texture_t *dst, const texture_t *texture, enum rt_texture_directory_type directory_type)
{
  assert(directory_type == RT_DIR_TYPE_GFX || directory_type == RT_DIR_TYPE_FLAT);

  char safe_name[sizeof(texture->name) + 1];
  memcpy(safe_name, texture->name, sizeof(texture->name));
  safe_name[sizeof(texture->name)] = '\0';

  // format+name+'\0' must be less
  assert((strlen(RT_TEXTURE_PATH_FORMAT(directory_type)) - 2) + strlen(safe_name) + 1 <= sizeof(dst->name));

  snprintf(dst->name, sizeof(dst->name), RT_TEXTURE_PATH_FORMAT(directory_type), safe_name);
}


// Returns initialized rt_texture_t btu with empty 'rg_handle'
static rt_texture_t *RT_Texture_RegisterPatch(const int lump, const rpatch_t *patch)
{
  if (patch->width == 0 || patch->height == 0)
  {
    assert_always("Patch has 0 side size");
    return NULL;
  }

  assert(lump >= 0 && lump < numlumps);
  int is_static = lumpinfo[lump].flags & LUMP_STATIC;

  rt_texture_t *td = RT_Texture_AllocEntry_T(lump, is_static ? rttextures.all_PFS_STATIC : rttextures.all_PFS);

  if (td == NULL)
  {
    return NULL;
  }

  assert(td->exists);
  td->id_type = is_static ? RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES_STATIC : RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES;
  td->id = lump;
  td->width = patch->width;
  td->height = patch->height;
  td->leftoffset = patch->leftoffset;
  td->topoffset = patch->topoffset;
  SetNameFromLump(td, lump, RT_DIR_TYPE_GFX);
  td->metainfo = RT_TextureMetaInfo_Find(td->name);
  td->flags |= td->metainfo ? td->metainfo->additional_flags : 0;

  // will be initialized by caller
  td->rg_handle = RG_NO_MATERIAL;
  return td;
}


static rt_texture_t *RT_Texture_RegisterFlat(const int lump_flat)
{
  const int lump = firstflat + lump_flat;

  assert(lump >= 0 && lump < numlumps);
  int is_static = lumpinfo[lump].flags & LUMP_STATIC;

  rt_texture_t *td = RT_Texture_AllocEntry_T(lump, is_static ? rttextures.all_PFS_STATIC : rttextures.all_PFS);

  if (td == NULL)
  {
    return NULL;
  }

  assert(td->exists);
  td->id_type = is_static ? RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES_STATIC : RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES;
  td->id = lump;
  td->width = 64;
  td->height = 64;
  td->leftoffset = 0;
  td->topoffset = 0;
  SetNameFromLump(td, lump, RT_DIR_TYPE_FLAT);
  td->metainfo = RT_TextureMetaInfo_Find(td->name);
  td->flags |= td->metainfo ? td->metainfo->additional_flags : 0;

  // will be initialized by caller
  td->rg_handle = RG_NO_MATERIAL;
  return td;
}


static rt_texture_t *RT_Texture_RegisterTexture(int texture_num, const texture_t *texture)
{
  assert(texture_num >= 0 && texture_num < numtextures);

  rt_texture_t *td = RT_Texture_AllocEntry_T(texture_num, rttextures.all_TXTR);

  if (td == NULL)
  {
    return NULL;
  }

  assert(td->exists);
  td->id_type = RT_TEXTURE_ID_TYPE_TEXTURES;
  td->id = texture_num;
  td->width = texture->width;
  td->height = texture->height;
  td->leftoffset = 0;
  td->topoffset = 0;
  SetNameFromTexture(td, texture, RT_DIR_TYPE_GFX);
  td->metainfo = RT_TextureMetaInfo_Find(td->name);
  td->flags |= td->metainfo ? td->metainfo->additional_flags : 0;

  // will be initialized by caller
  td->rg_handle = RG_NO_MATERIAL;
  return td;
}


static const rt_texture_t *RT_Texture_TryFindPatch(int lump)
{
  int is_static = lumpinfo[lump].flags & LUMP_STATIC;
  const rt_texture_t *td = is_static ? &rttextures.all_PFS_STATIC[lump] : &rttextures.all_PFS[lump];

  if (td->exists)
  {
    assert(td->id == lump);
    assert(td->width > 0 && td->height > 0);
    assert(
      (is_static && td->id_type == RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES_STATIC) ||
      (!is_static && td->id_type == RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES));
    
    return td;
  }

  return NULL;
}


static const rt_texture_t *RT_Texture_TryFindTexture(int texture_num)
{
  const rt_texture_t *td = &rttextures.all_TXTR[texture_num];

  if (td->exists)
  {
    assert(td->id_type == RT_TEXTURE_ID_TYPE_TEXTURES);
    assert(td->id == texture_num);
    assert(td->width > 0 && td->height > 0);

    return td;
  }

  return NULL;
}


static void DT_AddPatchToTexture_UnTranslated(uint8_t *buffer, const rpatch_t *patch, int originx, int originy)
{
  int paletted = 0;
  int texture_realtexwidth = patch->width;
  int texture_realtexheight = patch->height;
  int texture_buffer_width = patch->width;

  int x, y, j;
  int js, je;
  const rcolumn_t *column;
  const byte *source;
  int i, pos;

  if (!patch)
    return;

  const unsigned char *playpal = V_GetPlaypal();
  int xs = 0;
  int xe = patch->width;

  if ((xs + originx) >= texture_realtexwidth)
    return;
  if ((xe + originx) <= 0)
    return;
  if ((xs + originx) < 0)
    xs = -originx;
  if ((xe + originx) > texture_realtexwidth)
    xe += (texture_realtexwidth - (xe + originx));

  // if (patch->flags & PATCH_HASHOLES)
  //  gltexture->flags |= GLTEXTURE_HASHOLES;

  for (x = xs; x < xe; x++)
  {
  #ifdef RANGECHECK
    if (x >= patch->width)
    {
      lprintf(LO_ERROR, "AddPatchToTexture_UnTranslated x>=patch->width (%i >= %i)\n", x, patch->width);
      return;
    }
  #endif
    column = &patch->columns[x];
    for (i = 0; i < column->numPosts; i++)
    {
      const rpost_t *post = &column->posts[i];
      y = (post->topdelta + originy);
      js = 0;
      je = post->length;
      if ((js + y) >= texture_realtexheight)
        continue;
      if ((je + y) <= 0)
        continue;
      if ((js + y) < 0)
        js = -y;
      if ((je + y) > texture_realtexheight)
        je += (texture_realtexheight - (je + y));
      source = column->pixels + post->topdelta;
      if (paletted)
      {
        assert_always("gl_paletted_texture is always false for RT");
        //pos = (((js + y) * texture_buffer_width) + x + originx);
        //for (j = js; j < je; j++, pos += (texture_buffer_width))
        //{
        //#ifdef RANGECHECK
        //  if (pos >= gltexture->buffer_size)
        //  {
        //    lprintf(LO_ERROR, "AddPatchToTexture_UnTranslated pos>=size (%i >= %i)\n", pos + 3, gltexture->buffer_size);
        //    return;
        //  }
        //#endif
        //  buffer[pos] = gld_palmap[source[j]];
        //}
      }
      else
      {
        pos = 4 * (((js + y) * texture_buffer_width) + x + originx);
        for (j = js; j < je; j++, pos += (4 * texture_buffer_width))
        {
        #ifdef RANGECHECK
          if ((pos + 3) >= gltexture->buffer_size)
          {
            lprintf(LO_ERROR, "AddPatchToTexture_UnTranslated pos+3>=size (%i >= %i)\n", pos + 3, gltexture->buffer_size);
            return;
          }
        #endif
          //if (gl_boom_colormaps && use_boom_cm && !(comp[comp_skymap] && (gltexture->flags & GLTEXTURE_SKY)))
          //{
          //  const lighttable_t *colormap = (fixedcolormap ? fixedcolormap : fullcolormap);
          //  buffer[pos + 0] = playpal[colormap[source[j]] * 3 + 0];
          //  buffer[pos + 1] = playpal[colormap[source[j]] * 3 + 1];
          //  buffer[pos + 2] = playpal[colormap[source[j]] * 3 + 2];
          //}
          //else
          {
            buffer[pos + 0] = playpal[source[j] * 3 + 0];
            buffer[pos + 1] = playpal[source[j] * 3 + 1];
            buffer[pos + 2] = playpal[source[j] * 3 + 2];
          }
          buffer[pos + 3] = 255;
        }
      }
    }
  }
}


static void DT_AddFlatToTexture(uint8_t *buffer, const uint8_t *flat, int width, int height)
{
  int paletted = 0;
  int texture_realtexwidth = width;
  int texture_realtexheight = height;
  int texture_buffer_width = width;

  int x, y, pos;
  const unsigned char *playpal;

  if (!flat)
    return;
  //if (paletted)
  //{
  //  for (y = 0; y < texture_realtexheight; y++)
  //  {
  //    pos = (y * texture_buffer_width);
  //    for (x = 0; x < texture_realtexwidth; x++, pos++)
  //    {
  //    #ifdef RANGECHECK
  //      if (pos >= texture_buffer_size)
  //      {
  //        lprintf(LO_ERROR, "DT_AddFlatToTexture pos>=size (%i >= %i)\n", pos, texture_buffer_size);
  //        return;
  //      }
  //    #endif
  //      buffer[pos] = gld_palmap[flat[y * 64 + x]];
  //    }
  //  }
  //}
  //else
  {
    playpal = V_GetPlaypal();
    for (y = 0; y < texture_realtexheight; y++)
    {
      pos = 4 * (y * texture_buffer_width);
      for (x = 0; x < texture_realtexwidth; x++, pos += 4)
      {
      #ifdef RANGECHECK
        if ((pos + 3) >= gltexture->buffer_size)
        {
          lprintf(LO_ERROR, "DT_AddFlatToTexture pos+3>=size (%i >= %i)\n", pos + 3, texture_buffer_size);
          return;
        }
      #endif
        //e6y: Boom's color maps
        //if (gl_boom_colormaps && use_boom_cm)
        //{
        //  const lighttable_t *colormap = (fixedcolormap ? fixedcolormap : fullcolormap);
        //  buffer[pos + 0] = playpal[colormap[flat[y * 64 + x]] * 3 + 0];
        //  buffer[pos + 1] = playpal[colormap[flat[y * 64 + x]] * 3 + 1];
        //  buffer[pos + 2] = playpal[colormap[flat[y * 64 + x]] * 3 + 2];
        //}
        //else
        {
          buffer[pos + 0] = playpal[flat[y * 64 + x] * 3 + 0];
          buffer[pos + 1] = playpal[flat[y * 64 + x] * 3 + 1];
          buffer[pos + 2] = playpal[flat[y * 64 + x] * 3 + 2];
        }
        buffer[pos + 3] = 255;
      }
    }
  }
}


static void DT_AddPatchToTexture(unsigned char *buffer, const rpatch_t *patch)
{
  if (!patch)
    return;

  // force for RT
  int cm = CR_DEFAULT;
  int originx = 0;
  int originy = 0;

  assert(cm == CR_DEFAULT);
  DT_AddPatchToTexture_UnTranslated(buffer, patch, originx, originy);
}


static void MakeMonochrome(uint8_t *buffer, uint32_t w, uint32_t h)
{
  for (uint32_t i = 0; i < w * h; i++)
  {
    uint8_t r = buffer[i * 4 + 0];
    uint8_t g = buffer[i * 4 + 1];
    uint8_t b = buffer[i * 4 + 2];

    // get HSV's value
    uint8_t mono = i_max(i_max(r, g), b);

    buffer[i * 4 + 0] = mono;
    buffer[i * 4 + 1] = mono;
    buffer[i * 4 + 2] = mono;
  }
}


static dboolean HasAlpha(const uint8_t *buffer, uint32_t w, uint32_t h)
{
  for (uint32_t i = 0; i < w * h; i++)
  {
    if (buffer[i * 4 + 3] != 0xFF)
    {
      return true;
    }
  }
  return false;
}


RgMaterial BuildMaterial(const rt_texture_t *td, const uint8_t *rgba_buffer)
{
  RgStaticMaterialCreateInfo info =
  {
    .size = { td->width, td->height },
    .textures = {.albedoAlpha = {.isSRGB = true, .pData = rgba_buffer } },
    .pRelativePath = td->name,
    .filter = RG_SAMPLER_FILTER_NEAREST,
    .addressModeU = RG_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = RG_SAMPLER_ADDRESS_MODE_REPEAT,
  };

  RgMaterial m = RG_NO_MATERIAL;
  RgResult r = rgCreateStaticMaterial(rtmain.instance, &info, &m);
  RG_CHECK(r);

  return m;
}


const rt_texture_t *RT_Texture_GetFromPatchLump(int lump)
{
  {
    const rt_texture_t *existing = RT_Texture_TryFindPatch(lump);

    if (existing != NULL)
    {
      return existing;
    }
  }

  const rpatch_t *patch = R_CachePatchNum(lump);
  if (patch == NULL)
  {
    return NULL;
  }

  rt_texture_t *td = RT_Texture_RegisterPatch(lump, patch);
  if (td == NULL)
  {
    W_UnlockLumpNum(lump);
    return NULL;
  }
  
  uint32_t texture_buffer_size = td->width * td->height * 4;

  uint8_t *buffer = malloc(texture_buffer_size);
  memset(buffer, 0, texture_buffer_size);
  DT_AddPatchToTexture(buffer, patch);
  R_UnlockPatchNum(lump);


  if (td->flags & RT_TEXTURE_FLAG_MONOCHROME_FOR_COLORMAPS_BIT)
  {
    MakeMonochrome(buffer, td->width, td->height);
  }


  td->rg_handle = BuildMaterial(td, buffer);

  free(buffer);
  return td;
}

const rt_texture_t *RT_Texture_GetFromFlatLump(int lump_flat)
{
  int lump = firstflat + lump_flat;

  {
    const rt_texture_t *existing = RT_Texture_TryFindPatch(lump);

    if (existing != NULL)
    {
      return existing;
    }
  }

  const uint8_t *flat = W_CacheLumpNum(lump);
  if (flat == NULL)
  {
    return NULL;
  }

  rt_texture_t *td = RT_Texture_RegisterFlat(lump_flat);
  if (td == NULL)
  {
    W_UnlockLumpNum(lump);
    return NULL;
  }

  uint32_t texture_buffer_size = td->width * td->height * 4;
  uint8_t *buffer = malloc(texture_buffer_size);
  memset(buffer, 0, texture_buffer_size);
  DT_AddFlatToTexture(buffer, flat, td->width, td->height);
  W_UnlockLumpNum(lump);


  if (td->flags & RT_TEXTURE_FLAG_MONOCHROME_FOR_COLORMAPS_BIT)
  {
    MakeMonochrome(buffer, td->width, td->height);
  }


  td->rg_handle = BuildMaterial(td, buffer);

  free(buffer);
  return td;
}

const rt_texture_t *RT_Texture_GetFromTexture(int texture_num)
{
  {
    const rt_texture_t *existing = RT_Texture_TryFindTexture(texture_num);

    if (existing != NULL)
    {
      return existing;
    }
  }

  assert(texture_num >= 0 && texture_num < numtextures);
  const texture_t *texture = textures[texture_num];
  if (texture == NULL)
  {
    return NULL;
  }

  rt_texture_t *td = RT_Texture_RegisterTexture(texture_num, texture);
  if (td == NULL)
  {
    return NULL;
  }

  uint32_t texture_buffer_size = td->width * td->height * 4;
  uint8_t *buffer = malloc(texture_buffer_size);
  memset(buffer, 0, texture_buffer_size);
  const rpatch_t *patch = R_CacheTextureCompositePatchNum(texture_num);
  assert(patch->width == (int)td->width && patch->height == (int)td->height);
  DT_AddPatchToTexture(buffer, patch);
  R_UnlockTextureCompositePatchNum(texture_num);


  if (td->flags & RT_TEXTURE_FLAG_MONOCHROME_FOR_COLORMAPS_BIT)
  {
    MakeMonochrome(buffer, td->width, td->height);
  }
  if (HasAlpha(buffer, td->width, td->height))
  {
    td->flags |= RT_TEXTURE_FLAG_WITH_ALPHA_BIT;
  }


  td->rg_handle = BuildMaterial(td, buffer);

  free(buffer);
  return td;
}
