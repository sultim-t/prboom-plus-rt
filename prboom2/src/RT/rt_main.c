#include "rt_main.h"

#include <SDL_timer.h>

#include "doomtype.h"
#include "i_main.h"
#include "i_system.h"
#include "i_video.h"
#include "lprintf.h"
#include "m_argv.h"


rtmain_t rtmain = { 0 };


static void RT_Print(const char *pMessage, void *pUserData)
{
  lprintf(LO_ERROR, "%s\n", pMessage);
}


void RT_Init(HINSTANCE hinstance, HWND hwnd)
{
#define RESOURCES_BASE_PATH "C:\\Git\\Serious-Engine-RT\\Sources\\RTGL1\\Build\\"
  const char pShaderPath[] = RESOURCES_BASE_PATH;
  const char pBlueNoisePath[] = RESOURCES_BASE_PATH "BlueNoise_LDR_RGBA_128.ktx2";
  const char pWaterTexturePath[] = RESOURCES_BASE_PATH "WaterNormal_n.ktx2";

  RgWin32SurfaceCreateInfo win32Info =
  {
    .hinstance = hinstance,
    .hwnd = hwnd
  };

  RgInstanceCreateInfo info =
  {
    .pAppName = "GZDoom",
    .pAppGUID = "297e3cc1-4076-4a60-ac7c-5904c5db1313",

    .pWin32SurfaceInfo = &win32Info,

    .enableValidationLayer = M_CheckParm("-rtdebug"),
    .pfnPrint = RT_Print,

    .pShaderFolderPath = pShaderPath,
    .pBlueNoiseFilePath = pBlueNoisePath,
    .pWaterNormalTexturePath = pWaterTexturePath,

    .primaryRaysMaxAlbedoLayers = 1,
    .indirectIlluminationMaxAlbedoLayers = 1,
    .rayCullBackFacingTriangles = 1,

    .rasterizedMaxVertexCount = 1 << 20,
    .rasterizedMaxIndexCount = 1 << 21,
    .rasterizedVertexColorGamma = true,

    .rasterizedSkyMaxVertexCount = 1 << 16,
    .rasterizedSkyMaxIndexCount = 1 << 16,
    .rasterizedSkyCubemapSize = 256,

    .maxTextureCount = RG_MAX_TEXTURE_COUNT,
    .textureSamplerForceMinificationFilterLinear = true,

    .pOverridenTexturesFolderPath = "ovrd/mat/",
    .overridenAlbedoAlphaTextureIsSRGB = true,
    .overridenRoughnessMetallicEmissionTextureIsSRGB = false,
    .overridenNormalTextureIsSRGB = false,

    .vertexPositionStride = 3 * sizeof(float),
    .vertexNormalStride = 3 * sizeof(float),
    .vertexTexCoordStride = 2 * sizeof(float),
    .vertexColorStride = sizeof(uint32_t)
  };

  RgResult r = rgCreateInstance(&info, &rtmain.instance);
  RG_CHECK(r);

  rtmain.hwnd = hwnd;

  I_AtExit(RT_Destroy, true);
}

void RT_Destroy(void)
{
  RgResult r = rgDestroyInstance(rtmain.instance);
  RG_CHECK(r);

  memset(&rtmain, 0, sizeof(rtmain));
}

static double GetCurrentTime_Seconds()
{
  double current_tics = I_GetTime();
  double tics_per_second = TICRATE;

  // too low resolution
  return current_tics / tics_per_second;
}

static double GetCurrentTime_Seconds_Realtime()
{
  return SDL_GetTicks() / 1000.0;
}

static RgExtent2D GetCurrentHWNDSize()
{
  RgExtent2D extent = { 0,0 };

  RECT rect;
  if (GetWindowRect(rtmain.hwnd, &rect))
  {
    extent.width = rect.right - rect.left;
    extent.height = rect.bottom - rect.top;
  }

  assert(extent.width > 0 && extent.height > 0);
  return extent;
}

void RT_StartFrame(void)
{
  if (!window_focused)
  {
    return;
  }

  RgStartFrameInfo info =
  {
    .requestRasterizedSkyGeometryReuse = false,
    .requestShaderReload = false,
    .requestVSync = true,
    .surfaceSize = GetCurrentHWNDSize()
  };

  RgResult r = rgStartFrame(rtmain.instance, &info);
  RG_CHECK(r);
}

void RT_EndFrame()
{
  if (!window_focused)
  {
    return;
  }

  RgDrawFrameInfo info = {
    .worldUpVector = { 0,1,0 },
    .rayCullMaskWorld = 0x7,
    .rayLength = 10000.0f,
    .primaryRayMinDist = 0.1f,
    .disableRayTracing = false,
    .disableRasterization = false,
    .currentTime = GetCurrentTime_Seconds_Realtime(),
    .disableEyeAdaptation = false,
    .useSqrtRoughnessForIndirect = false,
  };

  //
  float identity[] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
  memcpy(info.view, identity, sizeof(identity));
  memcpy(info.projection, identity, sizeof(identity));
  info.fovYRadians = 3.14f;
  //

  RgResult r = rgDrawFrame(rtmain.instance, &info);
  RG_CHECK(r);
}

void RT_NewLevel(int gameepisode, int gamemap, int skytexture)
{
}

void RT_StartDrawScene()
{
}

void RT_DrawScene(player_t *player)
{
}

void RT_EndDrawScene()
{
}

void RT_ProjectSprite(mobj_t *thing, int lightlevel)
{
}

void RT_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel)
{
}

void RT_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling)
{
}

void RT_AddWall(seg_t *seg)
{
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

static void DrawQuad_Internal(RgMaterial mat, float x, float y, float width, float height, byte r, byte g, byte b)
{
  RgExtent2D ext = GetCurrentHWNDSize();

  const float vw = SCREENWIDTH;
  const float vh = SCREENHEIGHT;

  float x1 = x / vw * 2.0f - 1.0f;
  float y1 = y / vh * 2.0f - 1.0f;
  float x2 = (x + width ) / vw * 2.0f - 1.0f;
  float y2 = (y + height) / vh * 2.0f - 1.0f;

  float s1 = 0.0f;
  float t1 = 0.0f;
  float s2 = 1.0f;
  float t2 = 1.0f;

  uint32_t color = PackColor(r, g, b, 255);

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

void RT_DrawQuad(int x, int y, int width, int height, byte r, byte g, byte b)
{
  DrawQuad_Internal(RG_NO_MATERIAL, x, y, width, height, r, g, b);
}

void RT_DrawQuad_Flat(int lump, int x, int y, int width, int height, enum patch_translation_e flags)
{
  DrawQuad_Internal(RG_NO_MATERIAL, x, y, width, height, 255, 255, 255);
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

  DrawQuad_Internal(td->rg_handle, x, y, width, height, 255, 255, 255);
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

    xpos    = (x - leftoffset) * (float)params->video->width  / 320.0f + (float)params->deltax1;
    ypos    = (y - topoffset)  * (float)params->video->height / 200.0f + (float)params->deltay1;
    width   = (float)(td->width  * params->video->width)  / 320.0f;
    height  = (float)(td->height * params->video->height) / 200.0f;
  }
  else
  {
    xpos    = x - leftoffset;
    ypos    = y - topoffset;
    width   = (float)td->width;
    height  = (float)td->height;
  }
  
  DrawQuad_Internal(td->rg_handle, xpos, ypos, width, height, 255, 255, 255);
}

void RT_Wipe_DoMelt()
{
}

void RT_Wipe_ExitMelt()
{
}

void RT_Wipe_StartScreen()
{
}

void RT_Wipe_EndScreen()
{
}

void RT_OnMovePlane()
{
}

void RT_OnSkipDemoStop()
{
}

void RT_OnToggleFullscreen()
{
}

void RT_OnChangeScreenResolution()
{
}
