/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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

#ifndef _GL_STRUCT_H
#define _GL_STRUCT_H

extern int nodesVersion;

#define MAX_GLGAMMA 32
typedef enum
{
  gl_lightmode_glboom,
  gl_lightmode_gzdoom,
  gl_lightmode_mixed,

  gl_lightmode_last
} gl_lightmode_t;
extern gl_lightmode_t gl_lightmode;
extern const char *gl_lightmodes[];
extern int gl_light_ambient;
extern int useglgamma;
int gld_SetGammaRamp(int gamma);
void gld_CheckHardwareGamma(void);
void gld_FlushTextures(void);

extern int gl_seamless;
extern boolean gl_arb_multitexture;
extern boolean gl_arb_texture_compression;
extern boolean gl_ext_framebuffer_object;
extern boolean gl_ext_blend_color;
extern int render_canusedetail;
void gld_InitVertexData();
void gld_CleanVertexData();
void gld_UpdateSplitData(sector_t *sector);

//hack
extern int test_voodoo;

extern int gl_boom_colormaps;
extern int gl_boom_colormaps_default;

void gld_Init(int width, int height);
void gld_InitCommandLine(void);
void gld_InitTextureParams(void);

void gld_DrawNumPatch(int x, int y, int lump, int cm, enum patch_translation_e flags);
void gld_DrawBackground(const char* name);
void gld_DrawLine(int x0, int y0, int x1, int y1, int BaseColor);
void gld_DrawWeapon(int weaponlump, vissprite_t *vis, int lightlevel);
void gld_FillBlock(int x, int y, int width, int height, int col);
void gld_SetPalette(int palette);
void gld_ReadScreen (byte* scr);

void gld_CleanMemory(void);
void gld_PreprocessLevel(void);

void gld_Set2DMode();
void gld_InitDrawScene(void);
void gld_StartDrawScene(void);
void gld_AddPlane(int subsectornum, visplane_t *floor, visplane_t *ceiling);
void gld_AddWall(seg_t *seg);
void gld_AddSprite(vissprite_t *vspr);
void gld_DrawScene(player_t *player);
void gld_EndDrawScene(void);
void gld_Finish();

// wipe
int gld_wipe_doMelt(int ticks, int *y_lookup);
int gld_wipe_exitMelt(int ticks);
int gld_wipe_StartScreen(void);
int gld_wipe_EndScreen(void);

// hires
extern int gl_texture_usehires;
extern int gl_texture_usehires_default;
extern int gl_patch_usehires;
extern int gl_patch_usehires_default;
extern int gl_hires_override_pwads;
extern char *gl_texture_hires_dir;
int gld_PrecachePatches(void);

//clipper
boolean gld_clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle);
void gld_clipper_SafeAddClipRange(angle_t startangle, angle_t endangle);
void gld_clipper_Clear(void);
angle_t gld_FrustumAngle(void);

//missing flats (fake floors and ceilings)
extern boolean gl_use_stencil;
sector_t* GetBestFake(sector_t *sector, int ceiling, int validcount);

//vertical sync for GL
extern int gl_vsync;

#endif // _GL_STRUCT_H
