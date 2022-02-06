#include "rt_main.h"

#include <SDL_timer.h>
#include <GL/glu.h>

#include "doomtype.h"
#include "e6y.h"
#include "i_main.h"
#include "i_system.h"
#include "i_video.h"
#include "lprintf.h"
#include "m_argv.h"
#include "r_main.h"


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
    .rayCullBackFacingTriangles = 0,

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


static void Matrix_Multiply(float out[4][4], const float in1[4][4], const float in2[4][4])
{
  out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0] + in1[0][3] * in2[3][0];
  out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1] + in1[0][3] * in2[3][1];
  out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2] + in1[0][3] * in2[3][2];
  out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3] * in2[3][3];
  out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0] + in1[1][3] * in2[3][0];
  out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1] + in1[1][3] * in2[3][1];
  out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2] + in1[1][3] * in2[3][2];
  out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3] * in2[3][3];
  out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0] + in1[2][3] * in2[3][0];
  out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1] + in1[2][3] * in2[3][1];
  out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2] + in1[2][3] * in2[3][2];
  out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3] * in2[3][3];
  out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in1[3][3] * in2[3][0];
  out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in1[3][3] * in2[3][1];
  out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in1[3][3] * in2[3][2];
  out[3][3] = in1[3][0] * in2[0][3] + in1[3][1] * in2[1][3] + in1[3][2] * in2[2][3] + in1[3][3] * in2[3][3];
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


  // debug sun
  {
    RgDirectionalLightUploadInfo info = 
    {
      .color = { 1,1, 1 },
      .direction = { -1, -1, -1 },
      .angularDiameterDegrees = 0.5f
    };

    RgResult r = rgUploadDirectionalLight(rtmain.instance, &info);
    RG_CHECK(r);
  }


  RgDrawFrameSkyParams sky_params =
  {
    .skyType = RG_SKY_TYPE_COLOR,
    .skyColorDefault = {0.2f,0.2f,0.2f},
    .skyColorMultiplier = 1,
    .skyColorSaturation = 1,
    .skyViewerPosition = {0,0,0},
    .skyCubemap = RG_NO_MATERIAL,
    .skyCubemapRotationTransform = {0}
  };

  RgDrawFrameDebugParams debug_params =
  {
    .showMotionVectors = 0,
    .showGradients = 0,
    .showSectors = 0
  };

  RgDrawFrameInfo info = {
    .worldUpVector = { 0,1,0 },
    .fovYRadians = DEG2RAD(render_fovy),
    .rayCullMaskWorld = 0x7,
    .rayLength = 10000.0f,
    .primaryRayMinDist = 0.1f,
    .disableRayTracing = false,
    .disableRasterization = false,
    .currentTime = GetCurrentTime_Seconds_Realtime(),
    .disableEyeAdaptation = false,
    .useSqrtRoughnessForIndirect = false,
    .pSkyParams = &sky_params,
    .pDebugParams = &debug_params,
  };

#define FOV_FIX 2.0f
  static const float to_vk_projection[4][4] =
  {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f,-1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.5f * FOV_FIX, 0.5f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
  };
  float projMatrix_vk[4][4];
  Matrix_Multiply(projMatrix_vk, to_vk_projection, (const float(*)[4])projMatrix);


  memcpy(info.view, modelMatrix, sizeof(modelMatrix));
  memcpy(info.projection, projMatrix_vk, sizeof(projMatrix_vk));

  RgResult r = rgDrawFrame(rtmain.instance, &info);
  RG_CHECK(r);
}


void RT_NewLevel(int gameepisode, int gamemap, int skytexture)
{
  // RT_PreprocessLevel was already called
}


void RT_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel)
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
