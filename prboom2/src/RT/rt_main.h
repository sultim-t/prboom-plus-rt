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

#pragma once

#include "rt_common.h"
#include "rt_textures.h"
#include "rt_hud.h"

#include "d_player.h"
#include "r_defs.h"

typedef struct SDL_Window SDL_Window;

typedef struct
{
  RgInstance instance;

  SDL_Window *window;

  float mat_view[4][4];
  float mat_projectionvk[4][4];

  float mat_view_inverse[4][4];
  float mat_projectionvk_inverse[4][4];

  struct
  {
    const rt_texture_t *texture;
    float x_offset;
    float y_offset;
    dboolean gldwf_skyflip;
  } sky;

  dboolean request_wipe;
  float wipe_end_time;
  uint32_t powerupflags;

  dboolean request_flashlight;

  dboolean is_dlss_available;

  dboolean devmode;

} rtmain_t;

extern rtmain_t rtmain;


void RT_Init(void);
void RT_Destroy(void);

void RT_StartFrame(void);
void RT_EndFrame(void);

void RT_InitMatrices(const float viewMatrix[16], const float projMatrix[16]);
void RT_ProcessPlayer(const player_t *player);

double RT_GetCurrentTime(void);

void RT_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling);
void RT_AddWall(int subsectornum, seg_t *seg);
void RT_AddSprite(int sectornum, mobj_t *thing);
void RT_AddWeaponSprite(int weaponlump, const vissprite_t *vis, float zoffset);
void RT_AddSkyDome(void);

void RT_DrawLine(float x1, float y1, float x2, float y2, byte r, byte g, byte b);
void RT_DrawQuad(int x, int y, int width, int height, byte r, byte g, byte b, byte a);
void RT_DrawQuad_Patch(int lump, int x, int y, int width, int height, enum patch_translation_e flags);
void RT_DrawQuad_NumPatch(float x, float y, int lump, int cm /* use CM2RGB table for color */, enum patch_translation_e flags);
void RT_DrawQuad_Flat(int lump_flat, int x, int y, int width, int height, enum patch_translation_e flags);

void RT_StartScreenMelt(void);
enum rt_powerupflag_t {
  RT_POWERUP_FLAG_BERSERK_BIT = 1,
  RT_POWERUP_FLAG_DAMAGE_BIT = 2,
  RT_POWERUP_FLAG_RADIATIONSUIT_BIT = 4,
  RT_POWERUP_FLAG_BONUS_BIT = 8,
  RT_POWERUP_FLAG_INVUNERABILITY_BIT = 16,
  RT_POWERUP_FLAG_MORELIGHT_BIT = 32,
};
void RT_SetPowerupPalette(uint32_t powerupflags);


typedef struct
{
  int                vertex_count;
  RgPrimitiveVertex* vertices;
  int                index_count;
  uint32_t*          indices;
} rtsectordata_t;

void RT_PreprocessLevel(void);
rtsectordata_t RT_GetSectorGeometryData(int sectornum, dboolean is_ceiling);
void RT_GetLineInfo(int lineid, float *out_x1, float *out_z1, float *out_x2, float *out_z2);
void RT_DestroySectorGeometryData(const rtsectordata_t *data);

void RT_MapMetaInfo_Init(int mission);
void RT_MapMetaInfo_AddDelta(float deltaweight, int deltared, int deltagreen, int deltablue);
void RT_MapMetaInfo_WriteToFile(void);
dboolean RT_GetSectorLightLevelWeight(int sectornum, float *out_weight, RgColor4DPacked32 *out_color);


uint64_t RT_GetUniqueID_FirstPersonWeapon(int weaponindex);
uint64_t RT_GetUniqueID_Thing(const mobj_t *thing);
uint64_t RT_GetUniqueID_Wall(int lineid, int subsectornum, int drawwallindex);
uint64_t RT_GetUniqueID_Flat(int sectornum, dboolean ceiling);
uint64_t RT_GetUniqueID_Sky(void);


#define i_min(a,b) ((a) < (b) ? (a) : (b))
#define i_max(a,b) ((a) < (b) ? (b) : (a))
