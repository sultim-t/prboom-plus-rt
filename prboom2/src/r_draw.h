/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 * $Id: r_draw.h,v 1.6 2002/11/17 18:34:53 proff_fs Exp $
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
 *      System specific interface stuff.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_DRAW__
#define __R_DRAW__

#include "r_defs.h"
#include "v_video.h"

#ifdef __GNUG__
#pragma interface
#endif


//---------------------------------------------------------------------------
// Packaged into a struct - POPE
//---------------------------------------------------------------------------
typedef struct {
  int x;
  int yl;
  int yh;
  fixed_t iscale;
  fixed_t texturemid;
  int texheight;    // killough
  const byte *source; // first pixel in a column
  const byte *nextsource; // the next column to the right of source
  const lighttable_t *colormap; // the z-depth colormap
  const lighttable_t *nextcolormap; // the next farthest z-depth colormap
  fixed_t z; // the current column z coord
  fixed_t texu; // the current column u coord
  int topslope; // 0, -1, or 1 for specifying the slope of the top masked column edge
  int bottomslope; // 0, -1, or 1 for specifying the slope of the bottom masked column edge
  int drawingmasked; // 1 if R_DrawColumn* is currently drawing a masked column, otherwise 0
  int targetwidth; // normally SCREENWIDTH
  int targetheight; // normally SCREENHEIGHT
  const byte *translation;
} TRDrawColumnVars;

extern TRDrawColumnVars dcvars;

//---------------------------------------------------------------------------
// Packaged into a struct - POPE
//---------------------------------------------------------------------------
typedef struct {
  const byte *source; // start of a 64*64 tile image
  fixed_t z; // the current span z coord
  const lighttable_t *colormap;
  const lighttable_t *nextcolormap; // the next farthest z-depth colormap
  int     y;
  int     x1;
  int     x2;
  fixed_t xfrac;
  fixed_t yfrac;
  fixed_t xstep;
  fixed_t ystep;
} TRDrawSpanVars;

extern TRDrawSpanVars dsvars;

//---------------------------------------------------------------------------
// Used to specify what kind of filering you want
//---------------------------------------------------------------------------
typedef enum {
  RDRAW_FILTER_POINT,
  RDRAW_FILTER_LINEAR
} TRDrawFilterType;

//---------------------------------------------------------------------------
// Used to specify what kind of column edge rendering to use on masked 
// columns. SQUARE = standard, SLOPED = slope the column edge up or down
// based on neighboring columns
//---------------------------------------------------------------------------
typedef enum {
  RDRAW_MASKEDCOLUMNEDGE_SQUARE,
  RDRAW_MASKEDCOLUMNEDGE_SLOPED
} TRDrawColumnMaskedEdgeType;


//---------------------------------------------------------------------------
typedef struct {
  byte  *topleft_byte;
  short *topleft_short;
  int   *topleft_int;
  
  TRDrawFilterType filteruv;
  TRDrawFilterType filterz;
  TRDrawColumnMaskedEdgeType maskedColumnEdgeType;

  //-------------------------------------------------------------------------
  // Used to specify an early-out magnification threshold for filtering.
  // If a texture is being minified (dcvars.iscale > rdraw_magThresh), then it
  // drops back to point filtering.
  //-------------------------------------------------------------------------
  fixed_t magThresh;
  
} TRDrawVars;

extern TRDrawVars rdrawvars;

//---------------------------------------------------------------------------
// Used to fetch the right function for your pipeline needs. Will
// accomodate global point/linear uv and point/linear z settings.
//---------------------------------------------------------------------------
typedef void (__cdecl *TVoidFunc)();
typedef enum {
  RDRAW_PIPELINE_COL_STANDARD,
  RDRAW_PIPELINE_COL_TRANSLUCENT,
  RDRAW_PIPELINE_COL_TRANSLATED,
  RDRAW_PIPELINE_COL_FUZZ,
  RDRAW_PIPELINE_SPAN,
  RDRAW_PIPELINE_MAXFUNCS
} TRDrawPipelineType;

TVoidFunc R_GetDrawFunc(TRDrawPipelineType type);
TVoidFunc R_GetExactDrawFunc(TRDrawPipelineType type, TVidMode mode, TRDrawFilterType filteruv, TRDrawFilterType filterz);
//---------------------------------------------------------------------------

void R_VideoErase(unsigned ofs, int count);

extern byte playernumtotrans[MAXPLAYERS]; // CPhipps - what translation table for what player
extern byte *translationtables;


void R_InitBuffer(int width, int height);

// Initialize color translation tables, for player rendering etc.
void R_InitTranslationTables(void);

// Rendering function.
void R_FillBackScreen(void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder(void);

extern const byte *tranmap;         // translucency filter maps 256x256  // phares
extern const byte *main_tranmap;    // killough 4/11/98

#endif
