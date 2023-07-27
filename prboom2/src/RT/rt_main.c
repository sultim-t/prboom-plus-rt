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

#include "rt_main.h"

#include <SDL_timer.h>
#include <SDL_syswm.h>
#include <GL/glu.h>

#include "doomstat.h"
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


static void RT_Print(const char* pMessage, RgMessageSeverityFlags flags, void* pUserData)
{
    if (flags & RG_MESSAGE_SEVERITY_ERROR)
    {
        lprintf(LO_ERROR, "%s\n", pMessage);
    }
    else if (flags & RG_MESSAGE_SEVERITY_WARNING)
    {
        lprintf(LO_WARN, "%s\n", pMessage);
    }
    else
    {
        lprintf(LO_INFO, "%s\n", pMessage);
    }
}


void RT_Init()
{
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(sdl_window, &wmInfo);

#ifdef WIN32
  RgWin32SurfaceCreateInfo win32Info =
  {
    .hinstance = wmInfo.info.win.hinstance,
    .hwnd = wmInfo.info.win.window
  };
#else
  RgXlibSurfaceCreateInfo x11Info =
  {
    .dpy = wmInfo.info.x11.display,
    .window = wmInfo.info.x11.window
  };
#endif

  RgInstanceCreateInfo info = {
      .sType = RG_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = NULL,

      .pAppName = "PRBoom",
      .pAppGUID = "297e3cc1-4076-4a60-ac7c-5904c5db1313",

#if WIN32
      .pWin32SurfaceInfo = &win32Info,
#else
      .pXlibSurfaceCreateInfo = &x11Info,
#endif

      .pOverrideFolderPath = RG_RESOURCES_FOLDER,

      .pfnPrint        = RT_Print,
      .allowedMessages = M_CheckParm("-rtdebug")
                             ? (RG_MESSAGE_SEVERITY_VERBOSE | RG_MESSAGE_SEVERITY_INFO |
                                RG_MESSAGE_SEVERITY_WARNING | RG_MESSAGE_SEVERITY_ERROR)
                             : 0,

      .primaryRaysMaxAlbedoLayers          = 1,
      .indirectIlluminationMaxAlbedoLayers = 1,
      .rayCullBackFacingTriangles          = 0,
      .allowGeometryWithSkyFlag            = 1,

      .lightmapTexCoordLayerIndex = 1,

      .rasterizedMaxVertexCount   = 1 << 20,
      .rasterizedMaxIndexCount    = 1 << 21,
      .rasterizedVertexColorGamma = true,

      .rasterizedSkyCubemapSize = 256,

      .textureSamplerForceMinificationFilterLinear = true,
      .textureSamplerForceNormalMapFilterLinear    = true,

      .pbrTextureSwizzling = RG_TEXTURE_SWIZZLING_ROUGHNESS_METALLIC,

      .effectWipeIsUsed = true,

      .worldUp      = { 0, 1, 0 },
      .worldForward = { 0, 0, 1 },
      .worldScale   = 1.0f,
  };

  RgResult r = rgCreateInstance(&info, &rtmain.instance);
  if (r != RG_RESULT_SUCCESS)
  {
     I_Error("Can't initialize ray tracing engine: %s", rgUtilGetResultDescription(r));
     return;
  }

  if (rgUtilPackColorByte4D(255, 255, 255, 255) != RG_PACKED_COLOR_WHITE)
  {
     I_Error("Can't initialize ray tracing engine: Mismatching RG_PACKED_COLOR_WHITE constant");
     return;
  }

  rtmain.window = sdl_window;

#ifndef NDEBUG
  rtmain.devmode = true;
#else
  rtmain.devmode = M_CheckParm("-rtdevmode");
#endif


  {
        rtmain.is_dlss_available = rgUtilIsUpscaleTechniqueAvailable(
            rtmain.instance, RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS);
  }


  RT_Texture_Init();
  RT_MapMetaInfo_Init(gamemission);


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


/*
static double GetCurrentTime_Seconds()
{
  double current_tics = I_GetTime();
  double tics_per_second = TICRATE;

  return current_tics / tics_per_second;
}
*/


double RT_GetCurrentTime_Seconds_Realtime(void)
{
  return SDL_GetTicks() / 1000.0;
}


double RT_GetCurrentTime(void)
{
  // GetCurrentTime_Seconds is too low-resolution timer :(
  return RT_GetCurrentTime_Seconds_Realtime();
}


static RgExtent2D GetCurrentWindowSize()
{
  int w, h;
  SDL_GetWindowSize(rtmain.window, &w, &h);
  assert(w > 0 && h > 0);

  RgExtent2D extent = { w,h };
  return extent;
}


static dboolean IsCRTModeEnabled(rt_settings_renderscale_e renderscale)
{
  return renderscale == RT_SETTINGS_RENDERSCALE_320x200_CRT;
}


static RgExtent2D GetScaledResolution(rt_settings_renderscale_e renderscale)
{
  const RgExtent2D window_size = GetCurrentWindowSize();
  const double window_aspect = (double)window_size.width / (double)window_size.height;

  RgExtent2D original_doom = { 320, 200 };

  int y = SCREENHEIGHT;

  switch (renderscale)
  {
    case RT_SETTINGS_RENDERSCALE_320x200:
    {
      return original_doom;
    }
    case RT_SETTINGS_RENDERSCALE_320x200_CRT:
    {
      // double the resolution, to simulate interlacing
      original_doom.width *= 2;
      original_doom.height *= 2;
      return original_doom;
    }
    case RT_SETTINGS_RENDERSCALE_480:   y = 480; break;
    case RT_SETTINGS_RENDERSCALE_600:   y = 600; break;
    case RT_SETTINGS_RENDERSCALE_720:   y = 720; break;
    case RT_SETTINGS_RENDERSCALE_900:   y = 900; break;
    case RT_SETTINGS_RENDERSCALE_1080:  y = 1080; break;
    case RT_SETTINGS_RENDERSCALE_1200:  y = 1200; break;
    case RT_SETTINGS_RENDERSCALE_1440:  y = 1440; break;
    case RT_SETTINGS_RENDERSCALE_1600:  y = 1600; break;
    case RT_SETTINGS_RENDERSCALE_1920:  y = 1920; break;
    case RT_SETTINGS_RENDERSCALE_2160:  y = 2160; break;
    default: break;
  }

  RgExtent2D r = { (int)(window_aspect * (double)y), y };
  return r;
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
    case 1:   return RG_RENDER_RESOLUTION_MODE_QUALITY;
    case 2:   return RG_RENDER_RESOLUTION_MODE_BALANCED;
    case 3:   return RG_RENDER_RESOLUTION_MODE_PERFORMANCE;
    case 4:   return RG_RENDER_RESOLUTION_MODE_ULTRA_PERFORMANCE;
    default:  break;
  }

  return RG_RENDER_RESOLUTION_MODE_CUSTOM;
}


static void NormalizeRTSettings(rt_settings_t *settings)
{
  if (!rtmain.is_dlss_available)
  {
    settings->dlss = 0;
  }

  {
    RgExtent2D window_size = GetCurrentWindowSize();
    rt_settings_renderscale_e max_allowed = RT_SETTINGS_RENDERSCALE_NUM - 1;

    // must be in sync with rt_settings_renderscale_e
    static const int rs_height[] =
    {
      -1,-1,-1,480,600,720,900,1080,1200,1440,1600,1920,2160
    };
    // just to check that rs_height in sync with enum
    assert(RT_SETTINGS_RENDERSCALE_NUM == RG_ARRAY_SIZE(rs_height)); assert(RT_SETTINGS_RENDERSCALE_480 == 3 && RT_SETTINGS_RENDERSCALE_2160 == 12);

    for (int i = RT_SETTINGS_RENDERSCALE_NUM - 1; i >= 0; i--)
    {
      if ((int)window_size.height >= rs_height[i])
      {
        // next after closest
        max_allowed = i_min(i + 1, RT_SETTINGS_RENDERSCALE_NUM - 1);
        break;
      }
    }

    settings->renderscale = i_min(settings->renderscale, max_allowed);
  }
}


void RT_StartFrame(void)
{
  RgStartFrameInfo info = {
    .sType                  = RG_STRUCTURE_TYPE_START_FRAME_INFO,
    .pNext                  = NULL,
    .pMapName               = NULL,
    .ignoreExternalGeometry = false,
    .vsync                  = render_vsync,
  };

  RgResult r = rgStartFrame(rtmain.instance, &info);
  RG_CHECK(r);

  memset(&rtmain.sky, 0, sizeof(rtmain.sky));


  {
    rtmain.is_dlss_available =
        rgUtilIsUpscaleTechniqueAvailable(rtmain.instance, RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS);
  }


  NormalizeRTSettings(&rt_settings);


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


  RT_AddSkyDome();


  RgDrawFrameRenderResolutionParams resolution_params = {
      .sType                = RG_STRUCTURE_TYPE_DRAW_FRAME_RENDER_RESOLUTION_PARAMS,
      .pNext                = NULL,
      .upscaleTechnique     = rt_settings.dlss > 0  ? RG_RENDER_UPSCALE_TECHNIQUE_NVIDIA_DLSS
                              : rt_settings.fsr > 0 ? RG_RENDER_UPSCALE_TECHNIQUE_AMD_FSR2
                                                    : RG_RENDER_UPSCALE_TECHNIQUE_NEAREST,
      .sharpenTechnique     = RG_RENDER_SHARPEN_TECHNIQUE_NONE,
      .resolutionMode       = GetResolutionMode(rt_settings.dlss, rt_settings.fsr),
      .customRenderSize     = GetScaledResolution(rt_settings.renderscale),
      .pPixelizedRenderSize = NULL,
      .resetUpscalerHistory = false,
  };

  RgDrawFrameTonemappingParams tm_params = {
      .sType                = RG_STRUCTURE_TYPE_DRAW_FRAME_TONEMAPPING_PARAMS,
      .pNext                = &resolution_params,
      .disableEyeAdaptation = false,
      .ev100Min             = -4,
      .ev100Max             = 10,
      .luminanceWhitePoint  = 10,
      .saturation           = { 0, 0, 0 },
      .crosstalk            = { 1.0f, 1.0f, 1.0f },
  };

  RgDrawFrameReflectRefractParams reflrefr_params = {
      .sType                                 = RG_STRUCTURE_TYPE_DRAW_FRAME_REFLECT_REFRACT_PARAMS,
      .pNext                                 = &tm_params,
      .maxReflectRefractDepth                = rt_settings.refl_refr_max_depth,
      .typeOfMediaAroundCamera               = RG_MEDIA_TYPE_VACUUM,
      .indexOfRefractionGlass                = 1.52f,
      .indexOfRefractionWater                = 1.33f,
      .waterWaveSpeed                        = 0.05f, // for partial_invisibility
      .waterWaveNormalStrength               = 3.0f,  // for partial_invisibility
      .waterColor                            = { 0.25f, 0.4f, 0.6f },
      .waterWaveTextureDerivativesMultiplier = 1.0f,
      .waterTextureAreaScale                 = 1.0f,
      .portalNormalTwirl                     = false,
  };

  RgDrawFrameSkyParams sky_params = {
      .sType   = RG_STRUCTURE_TYPE_DRAW_FRAME_SKY_PARAMS,
      .pNext   = &reflrefr_params,
      .skyType = rtmain.sky.texture != NULL ? RG_SKY_TYPE_RASTERIZED_GEOMETRY : RG_SKY_TYPE_COLOR,
      .skyColorDefault    = { 0, 0, 0 },
      .skyColorMultiplier = 1.0f,
      .skyColorSaturation = 1.0f,
      .skyViewerPosition  = { 0, 0, 0 },
  };
#if RT_DOOM1_HACKS
  // make E3M1 less red
  if (gamemission == doom && gameepisode == 3 && gamemap == 1)
  {
    sky_params.skyColorSaturation = 0.7f;
  }
#endif

  RgDrawFrameBloomParams bloom_params = {
      .sType                   = RG_STRUCTURE_TYPE_DRAW_FRAME_BLOOM_PARAMS,
      .pNext                   = &sky_params,
      .bloomIntensity          = rt_settings.bloom_intensity == 0   ? -1
                                 : rt_settings.bloom_intensity == 1 ? 0.25f
                                                                    : 0.5f,
      .inputThreshold          = 4.0f,
      .bloomEmissionMultiplier = 9.0f,
      .lensDirtIntensity       = 1.0f,
  };

  RgDrawFrameIlluminationParams illum_params = {
      .sType                              = RG_STRUCTURE_TYPE_DRAW_FRAME_ILLUMINATION_PARAMS,
      .pNext                              = &bloom_params,
      .maxBounceShadows                   = 4,
      .enableSecondBounceForIndirect      = true,
      .cellWorldSize                      = 2.0f,
      .directDiffuseSensitivityToChange   = 1.0f,
      .indirectDiffuseSensitivityToChange = 0.75f,
      .specularSensitivityToChange        = 1.0f,
      .polygonalLightSpotlightFactor      = 2.0f,
      .lightUniqueIdIgnoreFirstPersonViewerShadows = NULL,
  };

  #define SCREEN_MELT_DURATION 1.5f
  RgPostEffectWipe ef_wipe =
  {
    .stripWidth = 1.0f / 320.0f,
    .beginNow = rtmain.request_wipe,
    .duration = SCREEN_MELT_DURATION
  };
  if (rtmain.request_wipe)
  {
    rtmain.wipe_end_time = (float)RT_GetCurrentTime() + SCREEN_MELT_DURATION;
    rtmain.request_wipe = false;
  }

  RgPostEffectRadialBlur ef_radialblur =
  {
    .isActive = rtmain.powerupflags & RT_POWERUP_FLAG_BERSERK_BIT,
    .transitionDurationIn = 0.4f,
    .transitionDurationOut = 3.0f
  };

  RgPostEffectChromaticAberration ef_chrabr =
  {
    .isActive = rtmain.powerupflags & RT_POWERUP_FLAG_DAMAGE_BIT,
    .transitionDurationIn = 0.05f,
    .transitionDurationOut = 0.3f,
    .intensity = 0.5f
  };

  RgPostEffectInverseBlackAndWhite ef_invbw =
  {
    .isActive = rtmain.powerupflags & RT_POWERUP_FLAG_INVUNERABILITY_BIT,
    .transitionDurationIn = 1.0f,
    .transitionDurationOut = 1.5f,
  };

  RgPostEffectHueShift ef_hueshift =
  {
    .isActive = rtmain.request_flashlight && (rtmain.powerupflags & RT_POWERUP_FLAG_MORELIGHT_BIT),
    .transitionDurationIn = 0.5f,
    .transitionDurationOut = 0.5f,
  };

  RgPostEffectDistortedSides ef_distortedsides =
  {
    .isActive = rtmain.powerupflags & RT_POWERUP_FLAG_RADIATIONSUIT_BIT,
    .transitionDurationIn = 1.0f,
    .transitionDurationOut = 1.0f,
  };

  RgPostEffectColorTint ef_tint_radsuit =
  {
    .isActive = true,
    .transitionDurationIn = 1.0f,
    .transitionDurationOut = 1.0f,
    .intensity = 1.0f,
    .color = { 0.2f, 1.0f, 0.4f }
  };
  RgPostEffectColorTint ef_tint_bonus =
  {
    .isActive = true,
    .transitionDurationIn = 0.0f,
    .transitionDurationOut = 0.7f,
    .intensity = 0.5f,
    .color = { 1.0f, 0.91f, 0.42f }
  };
  // static, so prev state's transition durations are preserved
  static RgPostEffectColorTint ef_tint = { 0 };
  {
    ef_tint.isActive = false;
    if (rtmain.powerupflags & RT_POWERUP_FLAG_RADIATIONSUIT_BIT)
    {
      ef_tint = ef_tint_radsuit;
    }
    else if (rtmain.powerupflags & RT_POWERUP_FLAG_BONUS_BIT)
    {
      ef_tint = ef_tint_bonus;
    }
  }

  RgPostEffectCRT ef_crt =
  {
    .isActive = IsCRTModeEnabled(rt_settings.renderscale)
  };

  RgDrawFramePostEffectsParams post_params = {
      .sType                 = RG_STRUCTURE_TYPE_DRAW_FRAME_POST_EFFECTS_PARAMS,
      .pNext                 = &illum_params,
      .pWipe                 = &ef_wipe,
      .pRadialBlur           = &ef_radialblur,
      .pChromaticAberration  = &ef_chrabr,
      .pInverseBlackAndWhite = &ef_invbw,
      .pHueShift             = &ef_hueshift,
      .pDistortedSides       = &ef_distortedsides,
      .pColorTint            = &ef_tint,
      .pCRT                  = &ef_crt,
  };

  RgDrawFrameInfo info = {
      .sType            = RG_STRUCTURE_TYPE_DRAW_FRAME_INFO,
      .pNext            = &post_params,
      .rayLength        = 10000.0f,
      .rayCullMaskWorld = RG_DRAW_FRAME_RAY_CULL_WORLD_0_BIT | RG_DRAW_FRAME_RAY_CULL_SKY_BIT,
      .disableRayTracedGeometry = false,
      .disableRasterization     = false,
      .presentPrevFrame         = false,
      .currentTime              = RT_GetCurrentTime(),
  };

  RgResult r = rgDrawFrame(rtmain.instance, &info);
  RG_CHECK(r);
}


void RT_StartScreenMelt()
{
  rtmain.request_wipe = true;
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

uint64_t RT_GetUniqueID_Sky(void) {
  return UNIQUE_TYPE_WEAPON | UINT32_MAX;
}
