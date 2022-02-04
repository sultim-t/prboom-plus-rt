#include "rt_main.h"

#include <SDL_timer.h>

#include "doomtype.h"
#include "i_main.h"
#include "i_system.h"
#include "i_video.h"
#include "lprintf.h"
#include "m_argv.h"


rtmain_t rtmain = { 0 };
static RgBool32 frame_started_guard = 0;


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

  RT_Texture_Init();

  I_AtExit(RT_Destroy, true);
}


void RT_Destroy(void)
{
  if (rtmain.instance == NULL)
  {
    return;
  }

  if (frame_started_guard)
  {
    RT_EndFrame();
  }

  RT_Texture_Destroy();

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

  frame_started_guard = true;
}


void RT_EndFrame()
{
  if (!frame_started_guard)
  {
    return;
  }
  frame_started_guard = false;

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
  RT_Texture_Destroy();
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
  subsector_t *subsector = &subsectors[subsectornum];
  if (subsector == NULL)
  {
    return;
  }

  if (floor != NULL)
  {
   // AddFlat(subsector->sector->iSectorID, floor);
  }

  if (ceiling != NULL)
  {
   // AddFlat(subsector->sector->iSectorID, ceiling);
  }
}


void RT_AddWall(seg_t *seg)
{
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
