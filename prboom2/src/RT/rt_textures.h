#pragma once

#include "rt_common.h"


typedef enum
{
} rt_texture_flag_bits_t;
typedef uint32_t rt_texture_flags_t;


typedef enum
{
  RT_TEXTURE_LUMP_TYPE_NONE,
  RT_TEXTURE_LUMP_TYPE_TEXTURES,
  RT_TEXTURE_LUMP_TYPE_PATCHES_FLATS_SPRITES,
  RT_TEXTURE_LUMP_TYPE_PATCHES_FLATS_SPRITES_STATIC,
} rt_texture_lump_type_t;


typedef struct rt_texture_t
{
  RgMaterial rg_handle;
  RgBool32 exists;

  rt_texture_lump_type_t lump_type;
  int lump_id;

  uint32_t width,height;
  int leftoffset, topoffset;
  rt_texture_flags_t flags;

} rt_texture_t;


void RT_Texture_Init(void);
void RT_Texture_Destroy(void);
const rt_texture_t *RT_Texture_GetFromPatchLump(int lump);
const rt_texture_t *RT_Texture_GetFromFlatLump(int lump_flat);
