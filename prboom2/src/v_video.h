/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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

extern byte      *screens[6];
extern int        usegamma;

//jff 4/24/98 loads color translation lumps
void V_InitColorTranslation(void);

// Allocates buffer screens, call before R_Init.
void V_Init (void);

enum patch_translation_e {
  VPT_NONE    = 0, // Normal
  VPT_FLIP    = 1, // Flip image horizontally
  VPT_TRANS   = 2, // Translate image via a translation table
  VPT_STRETCH = 4, // Stretch to compensate for high-res
};

#ifndef GL_DOOM
void V_CopyRect(int srcx,  int srcy,  int srcscrn, int width, int height,
                int destx, int desty, int destscrn,
                enum patch_translation_e flags);
#else
#define V_CopyRect(sx,sy,ss,w,h,dx,dy,ds,f)
#endif /* GL_DOOM */

#ifdef GL_DOOM
#define V_FillRect(s,x,y,w,h,c) gld_FillBlock(x,y,w,h,c)
#else
void V_FillRect(int scrn, int x, int y, int width, int height, byte colour);
#endif

// CPhipps - patch drawing
// Consolidated into the 3 really useful functions:
// V_DrawMemPatch - Draws the given patch_t
#ifdef GL_DOOM
#define V_DrawMemPatch(x,y,s,p,t,f) gld_DrawPatchFromMem(x,y,p,t,f)
#else
void V_DrawMemPatch(int x, int y, int scrn, const patch_t *patch,
        int cm, enum patch_translation_e flags);
#endif
// V_DrawNumPatch - Draws the patch from lump num
#ifdef GL_DOOM
#define V_DrawNumPatch(x,y,s,l,t,f) gld_DrawNumPatch(x,y,l,t,f)
#else
void V_DrawNumPatch(int x, int y, int scrn, int lump,
        int cm, enum patch_translation_e flags);
#endif
// V_DrawNamePatch - Draws the patch from lump "name"
#ifdef GL_DOOM
#define V_DrawNamePatch(x,y,s,n,t,f) gld_DrawNumPatch(x,y,W_GetNumForName(n),t,f)
#else
#define V_DrawNamePatch(x,y,s,n,t,f) V_DrawNumPatch(x,y,s,W_GetNumForName(n),t,f)
#endif

/* cph -
 * Functions to return width & height of a patch.
 * Doesn't really belong here, but is often used in conjunction with
 * this code.
 */
int V_NamePatchWidth(const char* name);
int V_NamePatchHeight(const char* name);

/* cphipps 10/99: function to tile a flat over the screen */
#ifdef GL_DOOM
#define V_DrawBackground(n,s) gld_DrawBackground(n)
#else
void V_DrawBackground(const char* flatname, int scrn);
#endif

// CPhipps - function to set the palette to palette number pal.
void V_SetPalette(int pal);

// CPhipps - function to plot a pixel

#ifndef GL_DOOM
#define V_PlotPixel(s,x,y,c) screens[s][x+SCREENWIDTH*y]=c
#endif

#define V_AllocScreen(scrn) screens[scrn] = malloc(SCREENWIDTH*SCREENHEIGHT)
#define V_FreeScreen(scrn) free(screens[scrn]); screens[scrn] = NULL

#ifdef GL_DOOM
#include "gl_struct.h"
#endif
#endif
