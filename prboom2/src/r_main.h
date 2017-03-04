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
 *      Renderer main interface.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"

#ifdef __GNUG__
#pragma interface
#endif

extern int r_frame_count;

//
// POV related.
//

extern fixed_t  viewcos;
extern fixed_t  viewsin;
extern fixed_t  viewtancos;
extern fixed_t  viewtansin;
extern int      viewwidth;
extern int      viewheight;
extern int      viewwindowx;
extern int      viewwindowy;
extern int      centerx;
extern int      centery;
extern fixed_t  globaluclip;
extern fixed_t  globaldclip;
extern fixed_t  centerxfrac;
extern fixed_t  centeryfrac;
extern fixed_t  yaspectmul;
extern fixed_t  viewheightfrac; //e6y: for correct cliping of things
extern fixed_t  projection;
extern fixed_t  skyiscale;
// e6y: wide-res
extern int wide_centerx;
extern int wide_offsetx;
extern int wide_offset2x;
extern int wide_offsety;
extern int wide_offset2y;
#define RMUL (1.6f/1.333333f)

// proff 11/06/98: Added for high-res
extern fixed_t  projectiony;
extern int      validcount;
// e6y: Added for more precise flats drawing
extern fixed_t viewfocratio;

//
// Rendering stats
//

extern int rendered_visplanes, rendered_segs, rendered_vissprites;
extern dboolean rendering_stats;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.

// SoM: I am really speechless at this... just... why?
// Lighting in doom was originally clamped off to just 16 brightness levels
// for sector lighting. Simply changing the constants is enough to change this
// it seriously bottles the mind why this wasn't done in doom from the start 
// except for maybe memory usage savings. 
#define LIGHTLEVELS_MAX   32

extern int LIGHTSEGSHIFT;
extern int LIGHTBRIGHT;
extern int LIGHTLEVELS;
extern int render_doom_lightmaps;

#define MAXLIGHTSCALE     48
#define LIGHTSCALESHIFT   12
#define MAXLIGHTZ        128
#define LIGHTZSHIFT       20

/* cph - allow crappy fake contrast to be disabled */
extern int fake_contrast;

// killough 3/20/98: Allow colormaps to be dynamic (e.g. underwater)
extern const lighttable_t *(*scalelight)[MAXLIGHTSCALE];
extern const lighttable_t *(*c_zlight)[LIGHTLEVELS_MAX][MAXLIGHTZ];
extern const lighttable_t *(*zlight)[MAXLIGHTZ];
extern const lighttable_t *fullcolormap;
extern int numcolormaps;    // killough 4/4/98: dynamic number of maps
extern const lighttable_t **colormaps;
// killough 3/20/98, 4/4/98: end dynamic colormaps

//e6y: for Boom colormaps in OpenGL mode
extern dboolean use_boom_cm;
extern int boom_cm;         // current colormap
extern int frame_fixedcolormap;

extern int          extralight;
extern const lighttable_t *fixedcolormap;

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.

#define NUMCOLORMAPS 32
// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP 32

//
// Utility functions.
//

PUREFUNC int R_PointOnSide(fixed_t x, fixed_t y, const node_t *node);
PUREFUNC int R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t *line);
angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x, fixed_t y);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);

//e6y: made more precise
angle_t R_PointToAngleEx(fixed_t x, fixed_t y);
angle_t R_PointToAngleEx2(fixed_t x1, fixed_t y1, fixed_t x, fixed_t y);
angle_t R_PointToPseudoAngle(fixed_t x, fixed_t y);

extern int r_have_internal_hires;

//
// REFRESH - the actual rendering functions.
//

void R_RenderPlayerView(player_t *player);   // Called by G_Drawer.
void R_Init(void);                           // Called by startup code.
void R_SetViewSize(int blocks);              // Called by M_Responder.
void R_ExecuteSetViewSize(void);             // cph - called by D_Display to complete a view resize

void R_ShowStats(void);
void R_ClearStats(void);

#define Pi 3.14159265358979323846f
#define DEG2RAD(a) ((a * Pi) / 180.0f)
#define RAD2DEG(a) ((a / Pi) * 180.0f)
#define MAP_COEFF 128.0f
#define MAP_SCALE (MAP_COEFF*(float)FRACUNIT)

extern int viewport[4];
extern float modelMatrix[16];
extern float projMatrix[16];
int R_Project(float objx, float objy, float objz, float *winx, float *winy, float *winz);

#endif
