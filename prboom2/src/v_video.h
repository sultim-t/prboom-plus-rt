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
 *  Gamma correction LUT.
 *  Color range translation support
 *  Functions to draw patches (by post) directly to screen.
 *  Functions to blit a block to the screen.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"
#include "doomdef.h"
// Needed because we are refering to patches.
#include "r_data.h"

//
// VIDEO
//

#define CENTERY     (SCREENHEIGHT/2)

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

// array of pointers to color translation tables
extern const byte *colrngs[];

// symbolic indices into color translation table pointer array
typedef enum
{
  CR_BRICK,   //0
  CR_TAN,     //1
  CR_GRAY,    //2
  CR_GREEN,   //3
  CR_BROWN,   //4
  CR_GOLD,    //5
  CR_RED,     //6
  CR_BLUE,    //7
  CR_ORANGE,  //8
  CR_YELLOW,  //9
  CR_BLUE2,   //10 // proff
  CR_LIMIT    //11 //jff 2/27/98 added for range check
} crange_idx_e;
//jff 1/16/98 end palette color range additions

#define CR_DEFAULT CR_RED   /* default value for out of range colors */

typedef struct {
  byte *data;          // pointer to the screen content
  boolean not_on_heap; // if set, no malloc or free is preformed and
                       // data never set to NULL. Used i.e. with SDL doublebuffer.
  int width;           // the width of the surface
  int height;          // the height of the surface, used when mallocing
  int pitch;           // tha actual width of one line, used when mallocing
} screeninfo_t;

#define NUM_SCREENS 6
extern screeninfo_t screens[NUM_SCREENS];
extern int          usegamma;

// The available bit-depth modes
typedef enum {
  VID_MODE8   = 0,
//  VID_MODE16  = 1,
//  VID_MODE32  = 2,
  VID_MODEGL  = 1
} video_mode_t;

extern video_mode_t default_videomode;

void V_InitMode(video_mode_t mode);

// video mode query interface
video_mode_t V_GetMode(void);

//jff 4/24/98 loads color translation lumps
void V_InitColorTranslation(void);

// Allocates buffer screens, call before R_Init.
void V_Init (void);

// V_CopyRect
typedef void (*V_CopyRect_f)(int srcx,  int srcy,  int srcscrn,
                             int width, int height,
                             int destx, int desty, int destscrn,
                             enum patch_translation_e flags);
extern V_CopyRect_f V_CopyRect;

// V_FillRect
typedef void (*V_FillRect_f)(int scrn, int x, int y,
                             int width, int height, byte colour);
extern V_FillRect_f V_FillRect;

// CPhipps - patch drawing
// Consolidated into the 3 really useful functions:

// V_DrawNumPatch - Draws the patch from lump num
typedef void (*V_DrawNumPatch_f)(int x, int y, int scrn,
                                 int lump, int cm,
                                 enum patch_translation_e flags);
extern V_DrawNumPatch_f V_DrawNumPatch;

// V_DrawNamePatch - Draws the patch from lump "name"
#define V_DrawNamePatch(x,y,s,n,t,f) V_DrawNumPatch(x,y,s,W_GetNumForName(n),t,f)

/* cph -
 * Functions to return width & height of a patch.
 * Doesn't really belong here, but is often used in conjunction with
 * this code.
 */
#define V_NamePatchWidth(name) R_NumPatchWidth(W_GetNumForName(name))
#define V_NamePatchHeight(name) R_NumPatchHeight(W_GetNumForName(name))

/* cphipps 10/99: function to tile a flat over the screen */
typedef void (*V_DrawBackground_f)(const char* flatname, int scrn);
extern V_DrawBackground_f V_DrawBackground;

// CPhipps - function to set the palette to palette number pal.
void V_SetPalette(int pal);

// CPhipps - function to plot a pixel

// V_PlotPixel
typedef void (*V_PlotPixel_f)(int,int,int,byte);
extern V_PlotPixel_f V_PlotPixel;

typedef struct
{
  int x, y;
} fpoint_t;

typedef struct
{
  fpoint_t a, b;
} fline_t;

// V_DrawLine
typedef void (*V_DrawLine_f)(fline_t* fl, int color);
extern V_DrawLine_f V_DrawLine;

void V_AllocScreen(screeninfo_t *scrn);
void V_AllocScreens();
void V_FreeScreen(screeninfo_t *scrn);
void V_FreeScreens();

#ifdef GL_DOOM
#include "gl_struct.h"
#endif
#endif
