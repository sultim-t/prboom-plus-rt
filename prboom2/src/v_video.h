/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: v_video.h,v 1.2 2000/05/07 20:19:34 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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
#ifdef GL_DOOM
#include "gl_struct.h"
#endif

//
// VIDEO
//

#define CENTERY     (SCREENHEIGHT/2)

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

/* cphipps 10/99 - fixed all pointers to pixel values to use "byte"
 *               - also fixed constness of most pointers
 * jff 2/16/98 palette color ranges for translation
 * jff 2/18/98 conversion to palette lookups for speed
 * jff 4/24/98 now pointers to lumps loaded 
 */
/*
extern const byte *cr_brick;
extern const byte *cr_tan;
extern const byte *cr_gray;
extern const byte *cr_green;
extern const byte *cr_brown;
extern const byte *cr_gold;
extern const byte *cr_red;
extern const byte *cr_blue;
extern const byte *cr_blue_status; /* killough 2/28/98 */
/*
extern const byte *cr_orange;
extern const byte *cr_yellow;
*/
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
extern int        dirtybox[4];
extern const byte gammatable[5][256];
extern int        usegamma;

//jff 4/24/98 loads color translation lumps
void V_InitColorTranslation(void);

// Allocates buffer screens, call before R_Init.
void V_Init (void);

void V_CopyRect(int srcx,  int srcy,  int srcscrn, int width, int height,
                int destx, int desty, int destscrn);

#ifdef GL_DOOM
#define V_FillRect(s,x,y,w,h,c) gld_FillBlock(x,y,w,h,c)
#else
void V_FillRect(int scrn, int x, int y, int width, int height, byte colour);
#endif

enum patch_translation_e {
  VPT_NONE    = 0, // Normal
  VPT_FLIP    = 1, // Flip image horizontally
  VPT_TRANS   = 2, // Translate image via a translation table
  VPT_STRETCH = 4, // Stretch to compensate for high-res
};

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
/*
void V_DrawNamePatch(int x, int y, int scrn, const char *name, 
		     const byte *trans, enum patch_translation_e flags);
*/
#endif

/* cph -
 * Functions to return width & height of a patch.
 * Doesn't really belong here, but is often used in conjunction with
 * this code.
 */
int V_NamePatchWidth(const char* name);
int V_NamePatchHeight(const char* name);

// Draw a linear block of pixels into the view buffer.

// CPhipps - added const's, patch translation flags for stretching
void V_DrawBlock(int x, int y, int scrn, int width, int height, 
		 const byte *src, enum patch_translation_e flags);

/* cphipps 10/99: function to tile a flat over the screen */
#ifdef GL_DOOM
#define V_DrawBackground gld_DrawBackground
#else
void V_DrawBackground(const char* flatname);
#endif

// Reads a linear block of pixels into the view buffer.

void V_GetBlock(int x, int y, int scrn, int width, int height, byte *dest);

void V_MarkRect(int x, int y, int width,int height);

// CPhipps - function to convert a patch_t into a simple block bitmap
// Returns pointer to the malloc()'ed bitmap, and its width and height
byte *V_PatchToBlock(const char* name, int cm, 
		     enum patch_translation_e flags, 
		     unsigned short* width, unsigned short* height);

// CPhipps - function to set the palette to palette number pal.
void V_SetPalette(int pal);

// CPhipps - function to plot a pixel
// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static const void V_PlotPixel(int scrn, int x, int y, byte colour) {
  screens[scrn][x+SCREENWIDTH*y] = colour;
}

#define V_AllocScreen(scrn) screens[scrn] = malloc(SCREENWIDTH*SCREENHEIGHT)
#define V_FreeScreen(scrn) free(screens[scrn]); screens[scrn] = NULL

#endif

//----------------------------------------------------------------------------
//
// $Log: v_video.h,v $
// Revision 1.2  2000/05/07 20:19:34  proff_fs
// changed use of colormaps from pointers to numbers.
// That's needed for OpenGL.
// The OpenGL part is slightly better now.
// Added some typedefs to reduce warnings in VisualC.
// Messages are also scaled now, because at 800x600 and
// above you can't read them even on a 21" monitor.
//
// Revision 1.1.1.1  2000/05/04 08:18:45  proff_fs
// initial login on sourceforge as prboom2
//
// Revision 1.14  2000/05/01 15:16:47  Proff
// added __inline for VisualC
//
// Revision 1.13  2000/04/04 10:55:33  cph
// Also add patch height function
//
// Revision 1.12  2000/04/04 10:03:00  cph
// New patch width function
//
// Revision 1.11  1999/10/27 18:38:03  cphipps
// Updated for W_Cache'd lumps being properly const
// Made colour translation tables be referenced by const byte*'s
// Updated various V_* functions for this change
//
// Revision 1.10  1999/10/27 11:59:49  cphipps
// Added V_DrawBackground, which draws a tiled flat over the screen
// (taken from M_DrawBackground and similar code in f_finale.c)
//
// Revision 1.9  1999/10/12 13:01:16  cphipps
// Changed header to GPL
//
// Revision 1.8  1999/08/30 15:16:39  cphipps
// V_FillRect prototype
// New functions to handle plotting a pixel, and allocating and
// deallocating screens
//
// Revision 1.7  1999/02/04 21:38:47  cphipps
// Extra pointer in screens[] ready for status bar scaling
//
// Revision 1.6  1998/12/31 20:19:42  cphipps
// New palette handling function decl added
//
// Revision 1.5  1998/12/31 14:14:00  cphipps
// Definitions for new V_Draw* functions
//
// Revision 1.4  1998/12/28 21:24:34  cphipps
// Don't allocate screens[2 to 3] in startup
// Made gamma correction tables const
//
// Revision 1.3  1998/12/24 20:42:03  cphipps
// Added V_DrawStretchedBlock
// Added const to source pointer of V_DrawBlock
//
// Revision 1.2  1998/11/17 16:03:57  cphipps
// Added hi-res additions
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.9  1998/05/06  11:12:54  jim
// Formattted v_video.*
//
// Revision 1.8  1998/05/03  22:53:58  killough
// beautification
//
// Revision 1.7  1998/04/24  08:09:44  jim
// Make text translate tables lumps
//
// Revision 1.6  1998/03/02  11:43:06  killough
// Add cr_blue_status for blue statusbar numbers
//
// Revision 1.5  1998/02/27  19:22:11  jim
// Range checked hud/sound card variables
//
// Revision 1.4  1998/02/19  16:55:06  jim
// Optimized HUD and made more configurable
//
// Revision 1.3  1998/02/17  23:00:41  jim
// Added color translation machinery and data
//
// Revision 1.2  1998/01/26  19:27:59  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:05  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------

