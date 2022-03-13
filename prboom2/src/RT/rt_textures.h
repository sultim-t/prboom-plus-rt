#pragma once

#include "rt_common.h"


typedef enum
{
  RT_TEXTURE_FLAG_WITH_ALPHA_BIT = 1,
  RT_TEXTURE_FLAG_IS_WATER_BIT = 2,
  RT_TEXTURE_FLAG_WITH_LIGHTSOURCE_BIT = 4,
  RT_TEXTURE_FLAG_WITH_VERTICAL_CONELIGHT_BIT = 8,
  RT_TEXTURE_FLAG_IS_EMISSIVE_BIT = 16,
  RT_TEXTURE_FLAG_WITHOUT_INDIR_ILLUMINATION_BIT = 32,
  RT_TEXTURE_FLAG_MONOCHROME_FOR_COLORMAPS_BIT = 64,
} rt_texture_flag_bits_t;
typedef uint32_t rt_texture_flags_t;


typedef enum
{
  RT_TEXTURE_ID_TYPE_NONE,
  RT_TEXTURE_ID_TYPE_TEXTURES,
  RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES,
  RT_TEXTURE_ID_TYPE_PATCHES_FLATS_SPRITES_STATIC,
} rt_texture_id_type_t;


#define RT_TEXTURE_NAME_MAX_LENGTH 28


typedef struct rt_texture_metainfo_t
{
  // These fields are internal
  char name[RT_TEXTURE_NAME_MAX_LENGTH];
  rt_texture_flags_t additional_flags;
  // Only use these values
  RgFloat3D light_color;
  float geom_emission;
  float falloff_multiplier;
} rt_texture_metainfo_t;


typedef struct rt_texture_t
{
  RgBool32 exists;
  char name[RT_TEXTURE_NAME_MAX_LENGTH];

  RgMaterial rg_handle;

  rt_texture_id_type_t id_type;
  int id;

  uint32_t width, height;
  int leftoffset, topoffset;
  rt_texture_flags_t flags;

  const rt_texture_metainfo_t *metainfo;

} rt_texture_t;


#define RT_TEXTURE_EMISSION(td) ((td) != NULL && ((td)->flags & RT_TEXTURE_FLAG_IS_EMISSIVE_BIT) && (td)->metainfo != NULL ? (td)->metainfo->geom_emission : 0.0f)


void RT_Texture_Init(void);
void RT_Texture_Destroy(void);
void RT_Texture_PrecacheTextures(void);
void RT_Texture_Clean_WithoutStatic(void);
void RT_Texture_Clean_Static(void);
void RT_TextureMetaInfo_Init(void);
const rt_texture_metainfo_t *RT_TextureMetaInfo_Find(const char *name);

const rt_texture_t *RT_Texture_GetFromPatchLump(int lump);
const rt_texture_t *RT_Texture_GetFromFlatLump(int lump_flat);
const rt_texture_t *RT_Texture_GetFromTexture(int texture_num);