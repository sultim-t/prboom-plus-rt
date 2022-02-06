#pragma once

#include "rt_common.h"
#include "rt_textures.h"

#include "d_player.h"
#include "r_defs.h"

typedef struct
{
  RgInstance instance;
  HWND hwnd;
} rtmain_t;

extern rtmain_t rtmain;


void RT_Init(HINSTANCE hinstance, HWND hwnd);
void RT_Destroy(void);

void RT_NewLevel(int gameepisode, int gamemap, int skytexture);

void RT_StartFrame(void);
void RT_EndFrame(void);

void RT_StartDrawScene(void);
void RT_DrawScene(player_t *player);
void RT_EndDrawScene(void);

void RT_ProjectSprite(mobj_t *thing, int lightlevel);
void RT_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel);

void RT_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling);
void RT_AddWall(seg_t *seg);

void RT_DrawLine(float x0, float y0, float x1, float y1, byte r, byte g, byte b);
void RT_DrawQuad(int x, int y, int width, int height, byte r, byte g, byte b);
void RT_DrawQuad_Patch(int lump, int x, int y, int width, int height, enum patch_translation_e flags);
void RT_DrawQuad_NumPatch(float x, float y, int lump, int cm /* use CM2RGB table for color */, enum patch_translation_e flags);
void RT_DrawQuad_Flat(int lump_flat, int x, int y, int width, int height, enum patch_translation_e flags);

// Interchange wipe effect
void RT_Wipe_DoMelt(void);
void RT_Wipe_ExitMelt(void);
void RT_Wipe_StartScreen(void);
void RT_Wipe_EndScreen(void);

// ?
void RT_OnMovePlane(void);
void RT_OnSkipDemoStop(void);
void RT_OnToggleFullscreen(void);
void RT_OnChangeScreenResolution(void);


typedef struct
{
  int vertex_count;
  RgFloat3D *positions;
  RgFloat3D *normals;
  RgFloat2D *texcoords;
  int index_count;
  uint32_t *indices;
  uint8_t *_internal_allocated;
} rtsectordata_t;

void RT_PreprocessLevel(void);
rtsectordata_t RT_CreateSectorGeometryData(int sectornum, dboolean is_ceiling);
void RT_GetLineInfo(int lineid, float *out_x1, float *out_z1, float *out_x2, float *out_z2);
void RT_DestroySectorGeometryData(const rtsectordata_t *data);
