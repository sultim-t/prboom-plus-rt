#pragma once

#include <assert.h>

#include "d_player.h"
#include "r_defs.h"

#include "RTGL1/RTGL1.h"


#define RG_CHECK(x) assert((x) == RG_SUCCESS)


typedef struct
{
  RgInstance instance;
} rtmain_t;

extern rtmain_t rtmain;


void RT_Init(HINSTANCE hinstance, HWND hwnd);
void RT_Destroy(void);

void RT_NewLevel(int gameepisode, int gamemap, int skytexture);

void RT_StartFrame(int window_width, int window_height);
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
void RT_DrawQuad_Flat(int lump, int x, int y, int width, int height, enum patch_translation_e flags);
void RT_DrawQuad_NumPatch(float x, float y, int lump, int cm /* use CM2RGB table for color */, enum patch_translation_e flags);

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
