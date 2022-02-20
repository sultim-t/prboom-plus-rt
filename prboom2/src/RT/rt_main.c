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
#include "p_setup.h"
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
    .pAppName = "PRBoom",
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
    .allowGeometryWithSkyFlag = 1,

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
  if (r != RG_SUCCESS)
  {
    I_Error("Can't initialize ray tracing engine");
    return;
  }

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


static float GetZNear()
{
  // from R_SetupMatrix
  extern int gl_nearclip;
  return (float)gl_nearclip / 100.0f;
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


static RgExtent2D GetScaledResolution(int renderscale)
{
  RgExtent2D window_size = GetCurrentHWNDSize();

  if (renderscale == RT_SETTINGS_RENDERSCALE_DEFAULT)
  {
    return window_size;
  }

  float f = 1.0f;
  switch(renderscale)
  {
    case 0: f = 0.5f; break;
    case 1: f = 0.6f; break;
    case 2: f = 0.75f; break;
    case 3: f = 0.9f; break;
    case 5: f = 1.1f; break;
    case 6: f = 1.25f; break;
    default: break;
  }

  RgExtent2D scaled_size = {
    .width  = BETWEEN(320, 3840, (int)(f * (float)window_size.width ) ),
    .height = BETWEEN(200, 2160, (int)(f * (float)window_size.height) ),
  };
  return scaled_size;
}


static RgRenderResolutionMode GetResolutionMode(int dlss, int fsr) // 0 - off, 1-4 - from highest to lowest
{
  // can't be both
  assert(!(dlss > 0 && fsr > 0));

  switch (dlss)
  {
    case 1:   return RG_RENDER_RESOLUTION_MODE_QUALITY;
    case 2:   return RG_RENDER_RESOLUTION_MODE_BALANCED;
    case 3:   return RG_RENDER_RESOLUTION_MODE_PERFORMANCE;
    case 4:   return RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE;
    default:  break;
  }

  switch (fsr)
  {
    case 1:   return RG_RENDER_RESOLUTION_MODE_ULTRA_QUALITY;
    case 2:   return RG_RENDER_RESOLUTION_MODE_QUALITY;
    case 3:   return RG_RENDER_RESOLUTION_MODE_BALANCED;
    case 4:   return RG_RENDER_RESOLUTION_MODE_PERFORMANCE;
    default:  break;
  }

  return RG_RENDER_RESOLUTION_MODE_CUSTOM;
}


static void NormalizeRTSettings(rt_settings_t *settings)
{
  RgBool32 dlss_available = false;
  RgResult r = rgIsRenderUpscaleTechniqueAvailable(rtmain.instance, RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS, &dlss_available);
  RG_CHECK(r);
  if (!dlss_available)
  {
    settings->dlss = 0;
  }

  // normalize resolution params
  /*if (settings->dlss > 0)
  {
    settings->fsr = 0;
    settings->renderscale = RT_SETTINGS_RENDERSCALE_DEFAULT;
  }
  else if (settings->fsr > 0)
  {
    settings->dlss = 0;
    settings->renderscale = RT_SETTINGS_RENDERSCALE_DEFAULT;
  }
  else if (settings->renderscale == RT_SETTINGS_RENDERSCALE_DEFAULT)
  {
    settings->fsr = 0;
    settings->dlss = 0;
  }*/
}


void RT_StartFrame(void)
{
  RgStartFrameInfo info =
  {
    .requestRasterizedSkyGeometryReuse = rtmain.was_new_sky ? false : true,
    .requestShaderReload = false,
    .requestVSync = true,
    .surfaceSize = GetCurrentHWNDSize()
  };

  RgResult r = rgStartFrame(rtmain.instance, &info);
  RG_CHECK(r);

  memset(&rtmain.sky, 0, sizeof(rtmain.sky));


  frame_started_guard = true;
}


void RT_EndFrame()
{
  if (!frame_started_guard)
  {
    return;
  }
  frame_started_guard = false;


  NormalizeRTSettings(&rt_settings);


#if RG_SKY_REUSE_BUG_HACK
  // TODO RT: rtgl1's 'reuse sky geomrty from prev frame' is broken for now:
  // there's a 1 frame sky camera delay; so keep 'was_new_sky' always true for now
  rtmain.was_new_sky = true;
#else
  rtmain.was_new_sky = false;
#endif


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

  RgDrawFrameRenderResolutionParams resolution_params =
  {
    .upscaleTechnique = rt_settings.dlss > 0 ? RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS : rt_settings.fsr > 0 ? RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR : RG_RENDER_UPSCALE_TECHNIQUE_LINEAR,
    .sharpenTechnique = RG_RENDER_SHARPEN_TECHNIQUE_NONE,
    .resolutionMode = GetResolutionMode(rt_settings.dlss, rt_settings.fsr),
    .renderSize = GetScaledResolution(rt_settings.renderscale)
  };

  RgDrawFrameTonemappingParams tm_params =
  {
    .minLogLuminance = -4,
    .maxLogLuminance = 0,
    .luminanceWhitePoint = 10 
  };

  RgDrawFrameReflectRefractParams reflrefr_params =
  {
    .isReflRefrAlphaTested = 1,
    .maxReflectRefractDepth = rt_settings.refl_refr_max_depth,
    .typeOfMediaAroundCamera = RG_MEDIA_TYPE_VACUUM,
    .reflectRefractCastShadows = 0,
    .reflectRefractToIndirect = 1,
    .indexOfRefractionGlass = 1.52f,
    .indexOfRefractionWater = 1.33f,
    .waterWaveSpeed = 0.05f, // for partial_invisibility
    .waterWaveNormalStrength = 3.0f, // for partial_invisibility
    .waterExtinction = { 0.030f, 0.019f, 0.013f },
    .waterWaveTextureDerivativesMultiplier = 1.0f,
    .waterTextureAreaScale = 1.0f
  };

  RgDrawFrameSkyParams sky_params =
  {
    .skyType = rtmain.sky.texture != NULL ? RG_SKY_TYPE_RASTERIZED_GEOMETRY : RG_SKY_TYPE_COLOR,
    .skyColorDefault = { 0,0,0 },
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
    .rayCullMaskWorld = RG_DRAW_FRAME_RAY_CULL_WORLD_0_BIT | RG_DRAW_FRAME_RAY_CULL_SKY_BIT,
    .rayLength = 10000.0f,
    .primaryRayMinDist = GetZNear(),
    .disableRayTracing = false,
    .disableRasterization = false,
    .currentTime = GetCurrentTime_Seconds_Realtime(),
    .disableEyeAdaptation = false,
    .useSqrtRoughnessForIndirect = false,
    .pRenderResolutionParams = &resolution_params,
    .pTonemappingParams = &tm_params,
    .pReflectRefractParams = &reflrefr_params,
    .pSkyParams = &sky_params,
    .pDebugParams = &debug_params,
  };
  memcpy(info.view, rtmain.mat_view, 16 * sizeof(float));
  memcpy(info.projection, rtmain.mat_projectionvk, 16 * sizeof(float));

  RgResult r = rgDrawFrame(rtmain.instance, &info);
  RG_CHECK(r);
}


void RT_NewLevel(int gameepisode, int gamemap, int skytexture)
{
  // RT_PreprocessLevel was already called

  RgResult r;

  r = rgStartNewScene(rtmain.instance);
  RG_CHECK(r);


  for (int iSectorID_A = 0; iSectorID_A < numsectors; iSectorID_A++)
  {
    for (int iSectorID_B = 0; iSectorID_B < numsectors; iSectorID_B++)
    {
      // based on P_CheckSight / P_CheckSight_12
      int pnum = iSectorID_A * numsectors + iSectorID_B;

      int bytenum = pnum >> 3;
      int bitnum = 1 << (pnum & 7);

      if (rejectmatrix[bytenum] & bitnum)
      {
        continue;
      }

      r = rgSetPotentialVisibility(rtmain.instance, iSectorID_A, iSectorID_B);
      RG_CHECK(r);
    }
  }

  r = rgSubmitStaticGeometries(rtmain.instance);
  RG_CHECK(r);


  rtmain.was_new_sky = true;
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


#define UNIQUE_TYPE_BITS_COUNT 2

#define UNIQUE_TYPE_MASK_FOR_IDS ((1ULL << (uint64_t)(64 - UNIQUE_TYPE_BITS_COUNT)) - 1ULL)
#define UNIQUE_TYPE_CHECK_IF_ID_VALID(id) assert(((id) & UNIQUE_TYPE_MASK_FOR_IDS) || ((id) == 0))

#define UNIQUE_TYPE_THING   (0ULL << (uint64_t)(64 - UNIQUE_TYPE_BITS_COUNT))
#define UNIQUE_TYPE_WALL    (1ULL << (uint64_t)(64 - UNIQUE_TYPE_BITS_COUNT))
#define UNIQUE_TYPE_FLAT    (2ULL << (uint64_t)(64 - UNIQUE_TYPE_BITS_COUNT))
#define UNIQUE_TYPE_WEAPON  (3ULL << (uint64_t)(64 - UNIQUE_TYPE_BITS_COUNT))


uint64_t RT_GetUniqueID_FirstPersonWeapon(int weaponindex)
{
  assert(weaponindex >= 0);

  UNIQUE_TYPE_CHECK_IF_ID_VALID(weaponindex);

  return UNIQUE_TYPE_WEAPON | weaponindex;
}


uint64_t RT_GetUniqueID_Thing(const mobj_t *thing)
{
  // assume that the same 'thing' doesn't change its address
  uint64_t address = (uint64_t)thing;
  uint64_t gid = address / sizeof(mobj_t);

  UNIQUE_TYPE_CHECK_IF_ID_VALID(gid);

  return UNIQUE_TYPE_THING | gid;
}


uint64_t RT_GetUniqueID_Wall(int lineid, int subsectornum, int drawwallindex)
{
  assert(lineid >= 0 && subsectornum >= 0 && drawwallindex >= 0);

  assert((uint64_t)lineid        < (1ULL << 32ULL));
  assert((uint64_t)subsectornum  < (1ULL << (56ULL - 32ULL)));
  assert((uint64_t)drawwallindex < (1ULL << 4ULL));

  uint64_t id = 
    ((uint64_t)lineid                ) | 
    ((uint64_t)subsectornum  << 32ULL) |
    ((uint64_t)drawwallindex << 56ULL);

  UNIQUE_TYPE_CHECK_IF_ID_VALID(id);
  return UNIQUE_TYPE_WALL | id;
}


uint64_t RT_GetUniqueID_Flat(int sectornum, dboolean ceiling)
{
  assert(sectornum >= 0);

  assert((uint64_t)sectornum < (1ULL << 32ULL));

  ceiling = ceiling ? 1 : 0;

  uint64_t id = 
    ((uint64_t)sectornum) | 
    ((uint64_t)ceiling << 32ULL);

  UNIQUE_TYPE_CHECK_IF_ID_VALID(id);
  return UNIQUE_TYPE_FLAT | id;
}


int RT_GetSectorNum_Fixed(fixed_t x, fixed_t y)
{
  return R_PointInSubsector(x, y)->sector->iSectorID;
}


int RT_GetSectorNum_Real(float real_x, float real_y)
{
  fixed_t x = (fixed_t)(real_x * MAP_SCALE);
  fixed_t y = (fixed_t)(real_y * MAP_SCALE);

  return RT_GetSectorNum_Fixed(x, y);
}


uint32_t RT_PackColor(byte r, byte g, byte b, byte a)
{
  return
    ((uint32_t)a << 24) |
    ((uint32_t)b << 16) |
    ((uint32_t)g << 8) |
    ((uint32_t)r);
}
