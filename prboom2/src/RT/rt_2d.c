#include "rt_main.h"

#include "i_video.h"
#include "v_video.h"


static float GetStatusBarScale()
{
  return 0.1f * BETWEEN(1, 10, rt_settings.statusbar_scale + 1);
}
static float GetHUDScale()
{
  return 0.1f * BETWEEN(1, 10, rt_settings.hud_scale + 1);
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

  uint32_t color = RT_PackColor(r, g, b, a);

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
    .renderType = RT_Get2DRenderType(),
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
  float f = 1.0f;


  if (flags & VPT_STATUSBAR)
  {
    f = GetStatusBarScale();
  }
  else if (flags & VPT_HUD)
  {
    f = GetHUDScale();
  }
  else
  {
    return;
  }
  enum patch_translation_e algn = flags & VPT_ALIGN_MASK;

#if RT_ENABLE_STATUS_BAR_SCALE
  float xpos = *p_xpos;
  float ypos = *p_ypos;
  float width = *p_width;
  float height = *p_height;

  const float vw = (float)SCREENWIDTH;
  const float vh = (float)SCREENHEIGHT;

  // to [0..1]
  float x1 = (xpos) / vw;
  float x2 = (xpos + width) / vw;
  float y1 = (ypos) / vh;
  float y2 = (ypos + height) / vh;

#define ANCHOR_CENTER_X   (x1)=(x1)*2-1;(x2)=(x2)*2-1
#define ANCHOR_CENTER_Y   (y1)=(y1)*2-1;(y2)=(y2)*2-1
#define DEANCHOR_CENTER_X (x1)=((x1)+1)/2;(x2)=((x2)+1)/2
#define DEANCHOR_CENTER_Y (y1)=((y1)+1)/2;(y2)=((y2)+1)/2

#define ANCHOR_LEFT_X     (x1)=(x1);(x2)=(x2)
#define ANCHOR_TOP_Y      (y1)=(y1);(y2)=(y2)
#define DEANCHOR_LEFT_X   (x1)=(x1);(x2)=(x2)
#define DEANCHOR_TOP_Y    (y1)=(y1);(y2)=(y2)

#define ANCHOR_RIGHT_X    (x1)=1-(x1);(x2)=1-(x2)
#define ANCHOR_BOTTOM_Y   (y1)=1-(y1);(y2)=1-(y2)
#define DEANCHOR_RIGHT_X  (x1)=1-(x1);(x2)=1-(x2)
#define DEANCHOR_BOTTOM_Y (y1)=1-(y1);(y2)=1-(y2)

  // RT: very special case for shrinking the classic status bar
  if (algn == VPT_ALIGN_BOTTOM)
  {
    ANCHOR_CENTER_X;
    ANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_TOP)
  {
    ANCHOR_CENTER_X;
    ANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_LEFT)
  {
    ANCHOR_LEFT_X;
    ANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT)
  {
    ANCHOR_RIGHT_X;
    ANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_BOTTOM)
  {
    ANCHOR_LEFT_X;
    ANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_TOP)
  {
    ANCHOR_LEFT_X;
    ANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_BOTTOM)
  {
    ANCHOR_RIGHT_X;
    ANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_TOP)
  {
    ANCHOR_RIGHT_X;
    ANCHOR_TOP_Y;
  }
  else
  {
    assert(0);
    return;
  }


  {
    x1 *= f; x2 *= f; y1 *= f; y2 *= f;
  }


  // reset anchor
  if (algn == VPT_ALIGN_BOTTOM)
  {
    DEANCHOR_CENTER_X;
    DEANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_TOP)
  {
    DEANCHOR_CENTER_X;
    DEANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_LEFT)
  {
    DEANCHOR_LEFT_X;
    DEANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT)
  {
    DEANCHOR_RIGHT_X;
    DEANCHOR_CENTER_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_BOTTOM)
  {
    DEANCHOR_LEFT_X;
    DEANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_LEFT_TOP)
  {
    DEANCHOR_LEFT_X;
    DEANCHOR_TOP_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_BOTTOM)
  {
    DEANCHOR_RIGHT_X;
    DEANCHOR_BOTTOM_Y;
  }
  else if (algn == VPT_ALIGN_RIGHT_TOP)
  {
    DEANCHOR_RIGHT_X;
    DEANCHOR_TOP_Y;
  }
  else
  {
    assert(0);
    return;
  }

  // back to screen
  xpos   = x1 * vw;
  width  = x2 * vw - xpos;
  ypos   = y1 * vh;
  height = y2 * vh - ypos;

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
