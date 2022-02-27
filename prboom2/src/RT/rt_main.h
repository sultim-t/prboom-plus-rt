#pragma once

#include "rt_common.h"
#include "rt_textures.h"
#include "rt_hud.h"

#include "d_player.h"
#include "r_defs.h"

typedef struct
{
  RgInstance instance;
  HWND hwnd;

  float mat_view[4][4];
  float mat_projectionvk[4][4];

  float mat_view_inverse[4][4];
  float mat_projectionvk_inverse[4][4];

  dboolean was_new_sky;
  struct
  {
    const rt_texture_t *texture;
    float x_offset;
    float y_offset;
    dboolean gldwf_skyflip;
  } sky;

  dboolean request_wipe;
  float wipe_start_time;

  dboolean request_shaderreload;

} rtmain_t;

extern rtmain_t rtmain;


void RT_Init(HINSTANCE hinstance, HWND hwnd);
void RT_Destroy(void);

void RT_StartFrame(void);
void RT_EndFrame(void);

void RT_InitMatrices(const float viewMatrix[16], const float projMatrix[16]);
void RT_ProcessPlayer(const player_t *player);
double RT_GetCurrentTime(void);

void RT_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling);
void RT_AddWall(int subsectornum, seg_t *seg);
void RT_AddSprite(int sectornum, mobj_t *thing, int lightlevel);
void RT_AddWeaponSprite(int weaponlump, vissprite_t *vis, int lightlevel);
void RT_AddSkyDome(void);

void RT_DrawLine(float x0, float y0, float x1, float y1, byte r, byte g, byte b);
void RT_DrawQuad(int x, int y, int width, int height, byte r, byte g, byte b, byte a);
void RT_DrawQuad_Patch(int lump, int x, int y, int width, int height, enum patch_translation_e flags);
void RT_DrawQuad_NumPatch(float x, float y, int lump, int cm /* use CM2RGB table for color */, enum patch_translation_e flags);
void RT_DrawQuad_Flat(int lump_flat, int x, int y, int width, int height, enum patch_translation_e flags);

void RT_StartScreenMelt(void);
dboolean RT_IsScreenMeltActive(void);

// TODO RT: unnecessary functions?
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

void RT_UploadStaticScene(void);
void RT_PreprocessLevel(void);
rtsectordata_t RT_CreateSectorGeometryData(int sectornum, dboolean is_ceiling);
void RT_GetLineInfo(int lineid, float *out_x1, float *out_z1, float *out_x2, float *out_z2);
void RT_DestroySectorGeometryData(const rtsectordata_t *data);


uint64_t RT_GetUniqueID_FirstPersonWeapon(int weaponindex);
uint64_t RT_GetUniqueID_Thing(const mobj_t *thing);
uint64_t RT_GetUniqueID_Wall(int lineid, int subsectornum, int drawwallindex);
uint64_t RT_GetUniqueID_Flat(int sectornum, dboolean ceiling);


int RT_GetSectorNum_Fixed(fixed_t x, fixed_t y);
int RT_GetSectorNum_Real(float real_x, float real_y);


uint32_t RT_PackColor(byte r, byte g, byte b, byte a);
