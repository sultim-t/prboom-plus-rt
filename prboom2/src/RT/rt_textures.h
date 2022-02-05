#pragma once

#include "rt_common.h"


typedef enum
{
} rt_texture_flag_bits_t;
typedef uint32_t rt_texture_flags_t;


typedef enum
{
  RT_TEXTURE_ID_TYPE_NONE,
  RT_TEXTURE_ID_TYPE_TEXTURES,
  RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES,
  RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES_STATIC,
} rt_texture_id_type_t;


typedef struct rt_texture_t
{
  RgMaterial rg_handle;
  RgBool32 exists;

  rt_texture_id_type_t id_type;
  int id;

  uint32_t width,height;
  int leftoffset, topoffset;
  rt_texture_flags_t flags;

} rt_texture_t;


void RT_Texture_Init(void);
void RT_Texture_Destroy(void);
void RT_Texture_PrecacheTextures(void);
void RT_Texture_Clean_WithoutStatic(void);
void RT_Texture_Clean_Static(void);
const rt_texture_t *RT_Texture_GetFromPatchLump(int lump);
const rt_texture_t *RT_Texture_GetFromFlatLump(int lump_flat);
const rt_texture_t *RT_Texture_GetFromTexture(int texture_num);
