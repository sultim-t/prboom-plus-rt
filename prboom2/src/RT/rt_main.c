#include "rt_main.h"

#include "doomtype.h"
#include "i_system.h"
#include "lprintf.h"
#include "m_argv.h"


rtmain_t rtmain = { 0 };


static void RT_Print(const char *pMessage, void *pUserData)
{
  lprintf(LO_ERROR, "%s\n", pMessage);
}


void RT_Init(void)
{
#define RESOURCES_BASE_PATH "C:\\Git\\Serious-Engine-RT\\Sources\\RTGL1\\Build\\"
  const char pShaderPath[] = RESOURCES_BASE_PATH;
  const char pBlueNoisePath[] = RESOURCES_BASE_PATH "BlueNoise_LDR_RGBA_128.ktx2";
  const char pWaterTexturePath[] = RESOURCES_BASE_PATH "WaterNormal_n.ktx2";

  RgWin32SurfaceCreateInfo win32Info =
  {
    .hinstance = NULL,
    .hwnd = NULL
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

    .maxTextureCount = 1024,
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

  I_AtExit(RT_Destroy, true);
}

void RT_Destroy(void)
{
  RgResult r = rgDestroyInstance(rtmain.instance);
  RG_CHECK(r);
}

void RT_NewLevel(int gameepisode, int gamemap, int skytexture)
{}

void RT_StartFrame(int window_width, int window_height)
{
  RgStartFrameInfo info =
  {
    .requestRasterizedSkyGeometryReuse = false,
    .requestShaderReload = false,
    .requestVSync = true,
    .surfaceSize = { window_width, window_height }
  };

  RgResult r = rgStartFrame(rtmain.instance, &info);
  RG_CHECK(r);
}

void RT_EndFrame()
{
  RgDrawFrameInfo info = {
    .worldUpVector = { 0,1,0 },
    .rayCullMaskWorld = 0x7,
    .rayLength = 10000.0f,
    .primaryRayMinDist = 0.1f,
    .disableRayTracing = false,
    .disableRasterization = false,
    .currentTime = static_cast<double>(FrameTime) / 1000.0,
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

void RT_DrawLine(float x0, float y0, float x1, float y1, byte r, byte g, byte b)
{
}

void RT_DrawQuad(int x, int y, int width, int height, byte r, byte g, byte b)
{
}

void RT_DrawQuad_Patch(int lump, int x, int y, int width, int height, enum patch_translation_e flags)
{
}

void RT_DrawQuad_Flat(int lump, int x, int y, int width, int height, enum patch_translation_e flags)
{
}

void RT_DrawQuad_NumPatch(float x, float y, int lump, int cm, enum patch_translation_e flags)
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
