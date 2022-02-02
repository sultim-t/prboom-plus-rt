#include "rt_main.h"

#include "lprintf.h"
#include "m_argv.h"


rtmain_t rtmain = { 0 };


static void RgPrint(const char *pMessage, void *pUserData)
{
	lprintf(LO_ERROR, "%s\n", pMessage);
}


static void Test()
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
		.pfnPrint = RgPrint,

		.pShaderFolderPath = pShaderPath,
		.pBlueNoiseFilePath = pBlueNoisePath,
		.pWaterNormalTexturePath = pWaterTexturePath,

		.primaryRaysMaxAlbedoLayers = 1,
		.indirectIlluminationMaxAlbedoLayers = 1,
		.rayCullBackFacingTriangles = 1,

		.rasterizedMaxVertexCount = 1 << 20,
		.rasterizedMaxIndexCount = 1 << 21,
		.rasterizedVertexColorGamma = TRUE,

		.rasterizedSkyMaxVertexCount = 1 << 16,
		.rasterizedSkyMaxIndexCount = 1 << 16,
		.rasterizedSkyCubemapSize = 256,

		.maxTextureCount = 1024,
		.textureSamplerForceMinificationFilterLinear = TRUE,

		.pOverridenTexturesFolderPath = "ovrd/mat/",
		.overridenAlbedoAlphaTextureIsSRGB = TRUE,
		.overridenRoughnessMetallicEmissionTextureIsSRGB = FALSE,
		.overridenNormalTextureIsSRGB = FALSE,

		.vertexPositionStride = 3 * sizeof(float),
		.vertexNormalStride = 3 * sizeof(float),
		.vertexTexCoordStride = 2 * sizeof(float),
		.vertexColorStride = sizeof(uint32_t)
	};

	RgResult r = rgCreateInstance(&info, &rtmain.instance);
	RG_CHECK(r);
}