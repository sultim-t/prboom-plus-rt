#include "rt_main.h"

#include "i_video.h"
#include "v_video.h"


static float GetStatusBarScale()
{
  switch(rt_settings.statusbar_scale)
  {
    case 0:   return 0.25f;
    case 1:   return 0.5f;
    case 2:   return 0.6f;
    case 3:   return 0.75f;
    case 4:   return 0.9f;
    default:  return 1.0f;
  }
}


static uint32_t PackColor(byte r, byte g, byte b, byte a)
{
  return
    ((uint32_t)a << 24) |
    ((uint32_t)b << 16) |
    ((uint32_t)g << 8) |
    ((uint32_t)r);
}


void RT_DrawLine(float x0, float y0, float x1, float y1, byte r, byte g, byte b)
{
  // TODO RT: drawing lines
}


//  0 -- 3
//  |    |
//  1 -- 2
static const uint32_t QUAD_INDICES[] = { 0, 1, 3, 3, 1, 2 };
static const float MATRIX_IDENTITY[] =
{
  1,0,0,0,
  0,1,0,0,
  0,0,1,0,
  0,0,0,1,
};


static void DrawQuad_Internal_T(RgMaterial mat,
                                float x, float y, float width, float height,
                                float s1, float t1, float s2, float t2,
                                byte r, byte g, byte b, byte a)
{
  const float vw = (float)SCREENWIDTH;
  const float vh = (float)SCREENHEIGHT;

  float x1 = x / vw * 2.0f - 1.0f;
  float y1 = y / vh * 2.0f - 1.0f;
  float x2 = (x + width) / vw * 2.0f - 1.0f;
  float y2 = (y + height) / vh * 2.0f - 1.0f;

  uint32_t color = PackColor(r, g, b, a);

  RgRasterizedGeometryVertexStruct verts[] =
  {
    { { x1, y1, 0 }, color, { s1, t1 } },
    { { x1, y2, 0 }, color, { s1, t2 } },
    { { x2, y1, 0 }, color, { s2, t1 } },
    { { x2, y1, 0 }, color, { s2, t1 } },
    { { x1, y2, 0 }, color, { s1, t2 } },
    { { x2, y2, 0 }, color, { s2, t2 } },
  };

  RgRasterizedGeometryUploadInfo info =
  {
    .renderType = RG_RASTERIZED_GEOMETRY_RENDER_TYPE_SWAPCHAIN,
    .vertexCount = RG_ARRAY_SIZE(verts),
    .pStructs = verts,
    .transform = RG_TRANSFORM_IDENTITY,
    .color = RG_COLOR_WHITE,
    .material = mat,
    .depthTest = false,
    .depthWrite = false,
    .blendEnable = true,
    .blendFuncSrc = RG_BLEND_FACTOR_SRC_ALPHA,
    .blendFuncDst = RG_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
  };

  RgResult _r = rgUploadRasterizedGeometry(rtmain.instance, &info, MATRIX_IDENTITY, NULL);
  RG_CHECK(_r);
}


static void DrawQuad_Internal(RgMaterial mat, float x, float y, float width, float height, byte r, byte g, byte b, byte a)
{
  DrawQuad_Internal_T(mat, x, y, width, height, 0, 0, 1, 1, r, g, b, a);
}


void RT_DrawQuad(int x, int y, int width, int height, byte r, byte g, byte b, byte a)
{
  DrawQuad_Internal(RG_NO_MATERIAL, (float)x, (float)y, (float)width, (float)height, r, g, b, a);
}


void RT_DrawQuad_Flat(int lump_flat, int x, int y, int width, int height, enum patch_translation_e flags)
{
  const rt_texture_t *td = RT_Texture_GetFromFlatLump(lump_flat);

  if (td == NULL)
  {
    return;
  }

  if (flags & VPT_STRETCH)
  {
    x = x * SCREENWIDTH / 320;
    y = y * SCREENHEIGHT / 200;
    width = width * SCREENWIDTH / 320;
    height = height * SCREENHEIGHT / 200;
  }

  float fU1 = 0;
  float fV1 = 0;

  float fU2 = (float)width / (float)td->width;
  float fV2 = (float)height / (float)td->height;

  DrawQuad_Internal_T(td->rg_handle, (float)x, (float)y, (float)width, (float)height, fU1, fV1, fU2, fV2, 255, 255, 255, 255);
}


void RT_DrawQuad_Patch(int lump, int x, int y, int width, int height, enum patch_translation_e flags)
{
  const rt_texture_t *td = RT_Texture_GetFromPatchLump(lump);

  if (td == NULL)
  {
    return;
  }

  x = x - td->leftoffset;
  y = y - td->topoffset;

  if (flags & VPT_STRETCH)
  {
    x = x * SCREENWIDTH / 320;
    y = y * SCREENHEIGHT / 200;
    width = width * SCREENWIDTH / 320;
    height = height * SCREENHEIGHT / 200;
  }

  DrawQuad_Internal(td->rg_handle, (float)x, (float)y, (float)width, (float)height, 255, 255, 255, 255);
}


void RT_TryApplyHUDCustomScale(enum patch_translation_e flags, float *p_xpos, float *p_ypos, float *p_width, float *p_height)
{
#if RT_ENABLE_STATUS_BAR_SCALE
  float xpos = *p_xpos;
  float ypos = *p_ypos;
  float width = *p_width;
  float height = *p_height;

  const float vw = (float)SCREENWIDTH;
  const float vh = (float)SCREENHEIGHT;

  // RT: very special case for shrinking the classic status bar
  if ((flags & VPT_ALIGN_BOTTOM) && (flags & VPT_STATUSBAR))
  {
    // anchor x around center
    float x1 =     (xpos         ) / vw * 2 - 1;
    float x2 =     (xpos + width ) / vw * 2 - 1;
    // anchor y around bottom
    float y1 = 1 - (ypos         ) / vh;
    float y2 = 1 - (ypos + height) / vh;

    float f = GetStatusBarScale();
    x1 *= f; x2 *= f; y1 *= f; y2 *= f;

    xpos   = (x1 + 1) / 2 * vw;
    width  = (x2 + 1) / 2 * vw - xpos;
    ypos   = (1 - y1) * vh;
    height = (1 - y2) * vh - ypos;
  }

  *p_xpos = xpos;
  *p_ypos = ypos;
  *p_width = width;
  *p_height = height;
#endif
}

void RT_DrawQuad_NumPatch(float x, float y, int lump, int cm, enum patch_translation_e flags)
{
  const rt_texture_t *td = RT_Texture_GetFromPatchLump(lump);

  if (td == NULL)
  {
    return;
  }

  float leftoffset, topoffset;

  if (flags & VPT_NOOFFSET)
  {
    leftoffset = 0;
    topoffset = 0;
  }
  else
  {
    leftoffset = (float)td->leftoffset;
    topoffset = (float)td->topoffset;
  }

  // [FG] automatically center wide patches without horizontal offset
  if (td->width > 320 && leftoffset == 0)
  {
    x -= (float)(td->width - 320) / 2;
  }

  float xpos, ypos;
  float width, height;

  if (flags & VPT_STRETCH_MASK)
  {
    const stretch_param_t *params = &stretch_params[flags & VPT_ALIGN_MASK];

    xpos = (x - leftoffset) * (float)params->video->width / 320.0f + (float)params->deltax1;
    ypos = (y - topoffset) * (float)params->video->height / 200.0f + (float)params->deltay1;
    width = (float)(td->width * params->video->width) / 320.0f;
    height = (float)(td->height * params->video->height) / 200.0f;
  }
  else
  {
    xpos = x - leftoffset;
    ypos = y - topoffset;
    width = (float)td->width;
    height = (float)td->height;
  }

  RT_TryApplyHUDCustomScale(flags, &xpos, &ypos, &width, &height);

  DrawQuad_Internal(td->rg_handle, xpos, ypos, width, height, 255, 255, 255, 255);
}
