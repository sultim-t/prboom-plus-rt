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
 *      The actual span/column drawing functions.
 *      Here find the main potential for optimization,
 *       e.g. inline assembly, different algorithms.
 *
 *-----------------------------------------------------------------------------*/

#include "doomstat.h"
#include "w_wad.h"
#include "r_main.h"
#include "v_video.h"
#include "st_stuff.h"
#include "g_game.h"
#include "am_map.h"
#include "lprintf.h"
#include "r_filter.h"
#include "r_draw.h"

#define MAXWIDTH  MAX_SCREENWIDTH          /* kilough 2/8/98 */
#define MAXHEIGHT MAX_SCREENHEIGHT

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//

int  viewwidth;
int  scaledviewwidth;
int  viewheight;
int  viewwindowx;
int  viewwindowy;

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//

// CPhipps - made const*'s
const byte *tranmap;          // translucency filter maps 256x256   // phares
const byte *main_tranmap;     // killough 4/11/98


//---------------------------------------------------------------------------
#define FUZZTABLE 50
#define FUZZOFF 1

static const int fuzzoffset[FUZZTABLE] = {
  FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
  FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
  FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
  FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
  FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
  FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
  FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

static int fuzzpos = 0;
byte *translationtables;

byte playernumtotrans[MAXPLAYERS];
extern lighttable_t *(*c_zlight)[LIGHTLEVELS][MAXLIGHTZ];

//---------------------------------------------------------------------------
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//
// Here is the version of R_DrawColumn that deals with translucent  // phares
// textures and sprites. It's identical to R_DrawColumn except      //    |
// for the spot where the color index is stuffed into *dest. At     //    V
// that point, the existing color index and the new color index
// are mapped through the TRANMAP lump filters to get a new color
// index whose RGB values are the average of the existing and new
// colors.
//
// Since we're concerned about performance, the 'translucent or
// opaque' decision is made outside this routine, not down where the
// actual code differences are.
//
// Moved R_DrawColumn.inl - POPE

//---------------------------------------------------------------------------
// Spectre/Invisibility.
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
// Moved R_DrawColumn.inl - POPE

//---------------------------------------------------------------------------
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
// Moved R_DrawColumn.inl - POPE

//---------------------------------------------------------------------------
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
// Moved R_DrawSpan.inl - POPE

//---------------------------------------------------------------------------
// These are the new R_Draw* functions. The main code was moved to 
// R_DrawColumn.inl and R_DrawSpan.inl to unify the pipelines and make this 
// simple switching possible. There are 4*3 versions of each function.
//
// Point = original point sampling
// Linear = Bilinear filtered sampling
// UV = texture UV
// Z = screen Z-depth
// 8 bit, 16 bit, and 32 bit
//
// - POPE
//---------------------------------------------------------------------------
// Pipeline macro flags for R_DrawColumn.inl and R_DrawSpan.inl
#define RDC_TRANSLUCENT    1
#define RDC_DITHERZ        2
#define RDC_BILINEAR       4
#define RDC_PLAYER         8
#define RDC_8BITS          16
#define RDC_16BITS         32
#define RDC_32BITS         64
#define RDC_FUZZ           128
#define RDC_ROUNDED        256

//---------------------------------------------------------------------------
// Set up the globals and their defaults
//---------------------------------------------------------------------------
TRDrawColumnVars dcvars;
TRDrawSpanVars dsvars;

TRDrawVars rdrawvars = { 
  0,0,0, // topleft
  RDRAW_FILTER_POINT, // filterwall
  RDRAW_FILTER_POINT, // filterfloor
  RDRAW_FILTER_POINT, // filterz
  RDRAW_MASKEDCOLUMNEDGE_SQUARE, // maskedColumnEdgeType
  // 49152 = FRACUNIT * 0.75
  // 81920 = FRACUNIT * 1.25
  81920 // magThresh
};

//---------------------------------------------------------------------------
static TVoidFunc getPointFilteredUVFunc(TRDrawPipelineType type);

//---------------------------------------------------------------------------
// R_DrawColum
//---------------------------------------------------------------------------
#undef  R_DRAWCOLUMN_FUNCTYPE
#define R_DRAWCOLUMN_FUNCTYPE RDRAW_PIPELINE_COL_STANDARD
// 8 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn8_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn8_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn8_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn8_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn8_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_ROUNDED)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn8_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_ROUNDED | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
// 16 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn16_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn16_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn16_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn16_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn16_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_ROUNDED)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn16_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_ROUNDED | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
// 32 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn32_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn32_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn32_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn32_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn32_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_ROUNDED)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawColumn32_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_ROUNDED | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"

//---------------------------------------------------------------------------
// R_DrawTLColum
//---------------------------------------------------------------------------
#undef  R_DRAWCOLUMN_FUNCTYPE
#define R_DRAWCOLUMN_FUNCTYPE RDRAW_PIPELINE_COL_TRANSLUCENT
// 8 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn8_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn8_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_TRANSLUCENT | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn8_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_TRANSLUCENT | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn8_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_TRANSLUCENT | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn8_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_ROUNDED | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn8_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_ROUNDED | RDC_DITHERZ | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
// 16 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn16_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn16_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_TRANSLUCENT | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn16_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_TRANSLUCENT | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn16_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_TRANSLUCENT | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn16_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_ROUNDED | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn16_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_ROUNDED | RDC_DITHERZ | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
// 32 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn32_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn32_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_TRANSLUCENT | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn32_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_TRANSLUCENT | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn32_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_TRANSLUCENT | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn32_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_ROUNDED | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTLColumn32_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_ROUNDED | RDC_DITHERZ | RDC_TRANSLUCENT)
#include "inl/r_drawcolumn.inl"

//---------------------------------------------------------------------------
// R_DrawTranslatedColumn
//---------------------------------------------------------------------------
#undef  R_DRAWCOLUMN_FUNCTYPE
#define R_DRAWCOLUMN_FUNCTYPE RDRAW_PIPELINE_COL_TRANSLATED
// 8 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn8_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn8_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_PLAYER | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn8_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_PLAYER | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn8_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_PLAYER | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn8_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_ROUNDED | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn8_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_ROUNDED | RDC_DITHERZ | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
// 16 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn16_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn16_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_PLAYER | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn16_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_PLAYER | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn16_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_PLAYER | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn16_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_ROUNDED | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn16_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_ROUNDED | RDC_DITHERZ | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
// 32 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn32_PointUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn32_PointUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_PLAYER | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn32_LinearUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_PLAYER | RDC_BILINEAR)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn32_LinearUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_PLAYER | RDC_BILINEAR | RDC_DITHERZ)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn32_RoundedUV_PointZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_ROUNDED | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"
#define R_DRAWCOLUMN_FUNCNAME R_DrawTranslatedColumn32_RoundedUV_LinearZ
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_ROUNDED | RDC_DITHERZ | RDC_PLAYER)
#include "inl/r_drawcolumn.inl"

//---------------------------------------------------------------------------
// R_DrawFuzzColumn
//---------------------------------------------------------------------------
#undef  R_DRAWCOLUMN_FUNCTYPE
#define R_DRAWCOLUMN_FUNCTYPE RDRAW_PIPELINE_COL_FUZZ
// 8 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawFuzzColumn8
#define R_DRAWCOLUMN_PIPELINE (RDC_8BITS | RDC_FUZZ)
#include "inl/r_drawcolumn.inl"
// 16 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawFuzzColumn16
#define R_DRAWCOLUMN_PIPELINE (RDC_16BITS | RDC_FUZZ)
#include "inl/r_drawcolumn.inl"
// 32 bit
#define R_DRAWCOLUMN_FUNCNAME R_DrawFuzzColumn32
#define R_DRAWCOLUMN_PIPELINE (RDC_32BITS | RDC_FUZZ)
#include "inl/r_drawcolumn.inl"

//---------------------------------------------------------------------------
// R_DrawSpan
//---------------------------------------------------------------------------
#undef  R_DRAWCOLUMN_FUNCTYPE
#define R_DRAWCOLUMN_FUNCTYPE RDRAW_PIPELINE_SPAN
// 8-bit
#define R_DRAWSPAN_FUNCNAME R_DrawSpan8_PointUV_PointZ
#define R_DRAWSPAN_PIPELINE (RDC_8BITS)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan8_PointUV_LinearZ
#define R_DRAWSPAN_PIPELINE (RDC_8BITS | RDC_DITHERZ)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan8_LinearUV_PointZ
#define R_DRAWSPAN_PIPELINE (RDC_8BITS | RDC_BILINEAR)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan8_LinearUV_LinearZ
#define R_DRAWSPAN_PIPELINE (RDC_8BITS | RDC_DITHERZ | RDC_BILINEAR)
#include "inl/r_drawspan.inl"
// 16-bit
#define R_DRAWSPAN_FUNCNAME R_DrawSpan16_PointUV_PointZ
#define R_DRAWSPAN_PIPELINE (RDC_16BITS)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan16_PointUV_LinearZ
#define R_DRAWSPAN_PIPELINE (RDC_16BITS | RDC_DITHERZ)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan16_LinearUV_PointZ
#define R_DRAWSPAN_PIPELINE (RDC_16BITS | RDC_BILINEAR)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan16_LinearUV_LinearZ
#define R_DRAWSPAN_PIPELINE (RDC_16BITS | RDC_DITHERZ | RDC_BILINEAR)
#include "inl/r_drawspan.inl"
// 32-bit
#define R_DRAWSPAN_FUNCNAME R_DrawSpan32_PointUV_PointZ
#define R_DRAWSPAN_PIPELINE (RDC_32BITS)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan32_PointUV_LinearZ
#define R_DRAWSPAN_PIPELINE (RDC_32BITS | RDC_DITHERZ)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan32_LinearUV_PointZ
#define R_DRAWSPAN_PIPELINE (RDC_32BITS | RDC_BILINEAR)
#include "inl/r_drawspan.inl"
#define R_DRAWSPAN_FUNCNAME R_DrawSpan32_LinearUV_LinearZ
#define R_DRAWSPAN_PIPELINE (RDC_32BITS | RDC_DITHERZ | RDC_BILINEAR)
#include "inl/r_drawspan.inl"

//---------------------------------------------------------------------------
// Lookup table for fetching the proper drawing function quickly
static TVoidFunc plFuncs[] = {
  // 8
  R_DrawColumn8_PointUV_PointZ,
  R_DrawColumn8_PointUV_LinearZ,
  R_DrawColumn8_LinearUV_PointZ,
  R_DrawColumn8_LinearUV_LinearZ,
  R_DrawColumn8_RoundedUV_PointZ,
  R_DrawColumn8_RoundedUV_LinearZ,
  
  R_DrawTLColumn8_PointUV_PointZ,
  R_DrawTLColumn8_PointUV_LinearZ,
  R_DrawTLColumn8_LinearUV_PointZ,
  R_DrawTLColumn8_LinearUV_LinearZ,
  R_DrawTLColumn8_RoundedUV_PointZ,
  R_DrawTLColumn8_RoundedUV_LinearZ,
  
  R_DrawTranslatedColumn8_PointUV_PointZ,
  R_DrawTranslatedColumn8_PointUV_LinearZ,
  R_DrawTranslatedColumn8_LinearUV_PointZ,
  R_DrawTranslatedColumn8_LinearUV_LinearZ,
  R_DrawTranslatedColumn8_RoundedUV_PointZ,
  R_DrawTranslatedColumn8_RoundedUV_LinearZ,

  R_DrawFuzzColumn8,
  R_DrawFuzzColumn8,
  R_DrawFuzzColumn8,
  R_DrawFuzzColumn8,
  R_DrawFuzzColumn8,
  R_DrawFuzzColumn8,
  
  R_DrawSpan8_PointUV_PointZ,
  R_DrawSpan8_PointUV_LinearZ, 
  R_DrawSpan8_LinearUV_PointZ,
  R_DrawSpan8_LinearUV_LinearZ,
  R_DrawSpan8_LinearUV_PointZ,
  R_DrawSpan8_LinearUV_LinearZ,

  // 16
  R_DrawColumn16_PointUV_PointZ,
  R_DrawColumn16_PointUV_LinearZ,
  R_DrawColumn16_LinearUV_PointZ,
  R_DrawColumn16_LinearUV_LinearZ,
  R_DrawColumn16_RoundedUV_PointZ,
  R_DrawColumn16_RoundedUV_LinearZ,
  
  R_DrawTLColumn16_PointUV_PointZ,
  R_DrawTLColumn16_PointUV_LinearZ,
  R_DrawTLColumn16_LinearUV_PointZ,
  R_DrawTLColumn16_LinearUV_LinearZ,
  R_DrawTLColumn16_RoundedUV_PointZ,
  R_DrawTLColumn16_RoundedUV_LinearZ,
  
  R_DrawTranslatedColumn16_PointUV_PointZ,
  R_DrawTranslatedColumn16_PointUV_LinearZ,
  R_DrawTranslatedColumn16_LinearUV_PointZ,
  R_DrawTranslatedColumn16_LinearUV_LinearZ,
  R_DrawTranslatedColumn16_RoundedUV_PointZ,
  R_DrawTranslatedColumn16_RoundedUV_LinearZ,

  R_DrawFuzzColumn16,
  R_DrawFuzzColumn16,
  R_DrawFuzzColumn16,
  R_DrawFuzzColumn16,
  R_DrawFuzzColumn16,
  R_DrawFuzzColumn16,
  
  R_DrawSpan16_PointUV_PointZ,
  R_DrawSpan16_PointUV_LinearZ, 
  R_DrawSpan16_LinearUV_PointZ,
  R_DrawSpan16_LinearUV_LinearZ,
  R_DrawSpan16_LinearUV_PointZ,
  R_DrawSpan16_LinearUV_LinearZ,
  
  // 32
  R_DrawColumn32_PointUV_PointZ,
  R_DrawColumn32_PointUV_LinearZ,
  R_DrawColumn32_LinearUV_PointZ,
  R_DrawColumn32_LinearUV_LinearZ,
  R_DrawColumn32_RoundedUV_PointZ,
  R_DrawColumn32_RoundedUV_LinearZ,
  
  R_DrawTLColumn32_PointUV_PointZ,
  R_DrawTLColumn32_PointUV_LinearZ,
  R_DrawTLColumn32_LinearUV_PointZ,
  R_DrawTLColumn32_LinearUV_LinearZ,
  R_DrawTLColumn32_RoundedUV_PointZ,
  R_DrawTLColumn32_RoundedUV_LinearZ,
  
  R_DrawTranslatedColumn32_PointUV_PointZ,
  R_DrawTranslatedColumn32_PointUV_LinearZ,
  R_DrawTranslatedColumn32_LinearUV_PointZ,
  R_DrawTranslatedColumn32_LinearUV_LinearZ,
  R_DrawTranslatedColumn32_RoundedUV_PointZ,
  R_DrawTranslatedColumn32_RoundedUV_LinearZ,

  R_DrawFuzzColumn32,
  R_DrawFuzzColumn32,
  R_DrawFuzzColumn32,
  R_DrawFuzzColumn32,
  R_DrawFuzzColumn32,
  R_DrawFuzzColumn32,
  
  R_DrawSpan32_PointUV_PointZ,
  R_DrawSpan32_PointUV_LinearZ, 
  R_DrawSpan32_LinearUV_PointZ,
  R_DrawSpan32_LinearUV_LinearZ,
  R_DrawSpan32_LinearUV_PointZ,
  R_DrawSpan32_LinearUV_LinearZ
};

//---------------------------------------------------------------------------
// Use the table above to get the requested pipeline function
//---------------------------------------------------------------------------
TVoidFunc R_GetExactDrawFunc(
TRDrawPipelineType type, int bitDepth, 
TRDrawFilterType filterwall,
TRDrawFilterType filterfloor,
TRDrawFilterType filterz
) {
  if (bitDepth == 16 && !vid_shortPalette) V_UpdateTrueColorPalette(VID_MODE16);
  if (bitDepth == 32 && !vid_intPalette) V_UpdateTrueColorPalette(VID_MODE32);
  if (type != RDRAW_PIPELINE_SPAN)
    return plFuncs[
      V_GetModeForNumBits(bitDepth)*(6*RDRAW_PIPELINE_MAXFUNCS) +
      type*6 +
      filterwall*2 +
      filterz
    ];
  else
    return plFuncs[
      V_GetModeForNumBits(bitDepth)*(6*RDRAW_PIPELINE_MAXFUNCS) +
      type*6 +
      filterfloor*2 +
      filterz
    ];
}

//---------------------------------------------------------------------------
static TVoidFunc getPointFilteredUVFunc(TRDrawPipelineType type) {
  // This lets us select point sampling when minifying and the global
  // filtering method when magnifying - POPE
  return R_GetExactDrawFunc(type, V_GetNumBits(), RDRAW_FILTER_POINT, RDRAW_FILTER_POINT, rdrawvars.filterz);
}

//---------------------------------------------------------------------------
TVoidFunc R_GetDrawFunc(TRDrawPipelineType type) {
  return R_GetExactDrawFunc(type, V_GetNumBits(), rdrawvars.filterwall, rdrawvars.filterfloor, rdrawvars.filterz);
}

//---------------------------------------------------------------------------
// R_InitBuffer
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//---------------------------------------------------------------------------
void R_InitBuffer(int width, int height) {
  int i=0;
  // Handle resize,
  //  e.g. smaller view windows
  //  with border and/or status bar.
  viewwindowx = (SCREENWIDTH-width) >> 1;

  // Same with base row offset.
  viewwindowy = width==SCREENWIDTH ? 0 : (SCREENHEIGHT-(ST_SCALED_HEIGHT-1)-height)>>1;

  rdrawvars.topleft_byte = (byte*)(screens[0].data) + viewwindowy*SCREENWIDTH + viewwindowx;
  rdrawvars.topleft_short = (short*)(screens[0].data) + viewwindowy*SCREENWIDTH + viewwindowx;
  rdrawvars.topleft_int = (int*)(screens[0].data) + viewwindowy*SCREENWIDTH + viewwindowx;

  dcvars.targetwidth = SCREENWIDTH;
  dcvars.targetheight = SCREENHEIGHT;
}

//---------------------------------------------------------------------------
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//---------------------------------------------------------------------------
void R_InitTranslationTables (void) {
  int i, j;
#define MAXTRANS 3
  byte transtocolour[MAXTRANS];

  // killough 5/2/98:
  // Remove dependency of colormaps aligned on 256-byte boundary

  if (translationtables == NULL) // CPhipps - allow multiple calls
    translationtables = malloc(256*MAXTRANS);

  for (i=0; i<MAXTRANS; i++) transtocolour[i] = 255;

  for (i=0; i<MAXPLAYERS; i++) {
    byte wantcolour = mapcolor_plyr[i];
    playernumtotrans[i] = 0;
    if (wantcolour != 0x70) // Not green, would like translation
      for (j=0; j<MAXTRANS; j++)
  if (transtocolour[j] == 255) {
    transtocolour[j] = wantcolour; playernumtotrans[i] = j+1; break;
  }
  }

  // translate just the 16 green colors
  for (i=0; i<256; i++)
    if (i >= 0x70 && i<= 0x7f)
      {
  // CPhipps - configurable player colours
        translationtables[i] = colormaps[0][((i&0xf)<<9) + transtocolour[0]];
        translationtables[i+256] = colormaps[0][((i&0xf)<<9) + transtocolour[1]];
        translationtables[i+512] = colormaps[0][((i&0xf)<<9) + transtocolour[2]];
      }
    else  // Keep all other colors as is.
      translationtables[i]=translationtables[i+256]=translationtables[i+512]=i;
}

//---------------------------------------------------------------------------
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
// CPhipps - patch drawing updated
//---------------------------------------------------------------------------
void R_FillBackScreen(void) {
  int     x,y;

  if (scaledviewwidth == SCREENWIDTH)
    return;

  V_DrawBackground(gamemode == commercial ? "GRNROCK" : "FLOOR7_2", 1);

  for (x=0; x<scaledviewwidth; x+=8)
    V_DrawNamePatch(viewwindowx+x,viewwindowy-8,1,"brdr_t", CR_DEFAULT, VPT_NONE);

  for (x=0; x<scaledviewwidth; x+=8)
    V_DrawNamePatch(viewwindowx+x,viewwindowy+viewheight,1,"brdr_b", CR_DEFAULT, VPT_NONE);

  for (y=0; y<viewheight; y+=8)
    V_DrawNamePatch(viewwindowx-8,viewwindowy+y,1,"brdr_l", CR_DEFAULT, VPT_NONE);

  for (y=0; y<viewheight; y+=8)
    V_DrawNamePatch(viewwindowx+scaledviewwidth,viewwindowy+y,1,"brdr_r", CR_DEFAULT, VPT_NONE);

  // Draw beveled edge.
  V_DrawNamePatch(viewwindowx-8,viewwindowy-8,1,"brdr_tl", CR_DEFAULT, VPT_NONE);

  V_DrawNamePatch(viewwindowx+scaledviewwidth,viewwindowy-8,1,"brdr_tr", CR_DEFAULT, VPT_NONE);

  V_DrawNamePatch(viewwindowx-8,viewwindowy+viewheight,1,"brdr_bl", CR_DEFAULT, VPT_NONE);

  V_DrawNamePatch(viewwindowx+scaledviewwidth,viewwindowy+viewheight,1,"brdr_br", CR_DEFAULT, VPT_NONE);
}

//---------------------------------------------------------------------------
// Copy a screen buffer.
//---------------------------------------------------------------------------
void R_VideoErase(unsigned ofs, int count) {
  if (V_GetMode() != VID_MODEGL)
    memcpy((byte*)screens[0].data+ofs*V_GetDepth(), (byte*)screens[1].data+ofs*V_GetDepth(), count*V_GetDepth());   // LFB copy.
}

//---------------------------------------------------------------------------
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//---------------------------------------------------------------------------
void R_DrawViewBorder(void) {
  if (V_GetMode() == VID_MODEGL) {
    R_FillBackScreen();
  } else {
  // proff 11/99: we don't have a backscreen in OpenGL from where we can copy this

    int top, side, ofs, i;
    // proff/nicolas 09/20/98: Added for high-res (inspired by DosDOOM)
    int side2;

    // proff/nicolas 09/20/98: Removed for high-res
    //  if (scaledviewwidth == SCREENWIDTH)
    //  return;

    // proff/nicolas 09/20/98: Added for high-res (inspired by DosDOOM)
    if ((SCREENHEIGHT != viewheight) ||
        ((automapmode & am_active) && ! (automapmode & am_overlay)))
    {
      ofs = ( SCREENHEIGHT - ST_SCALED_HEIGHT ) * SCREENWIDTH;
      side= ( SCREENWIDTH - ST_SCALED_WIDTH ) / 2;
      side2 = side * 2;

      R_VideoErase ( ofs, side );

      ofs += ( SCREENWIDTH - side );
      for ( i = 1; i < ST_SCALED_HEIGHT; i++ )
      {
        R_VideoErase ( ofs, side2 );
        ofs += SCREENWIDTH;
      }

      R_VideoErase ( ofs, side );
    }

    if ( viewheight >= ( SCREENHEIGHT - ST_SCALED_HEIGHT ))
      return; // if high-res, don´t go any further!

    top = ((SCREENHEIGHT-ST_SCALED_HEIGHT)-viewheight)/2;
    side = (SCREENWIDTH-scaledviewwidth)/2;

    // copy top and one line of left side
    R_VideoErase (0, top*SCREENWIDTH+side);

    // copy one line of right side and bottom
    ofs = (viewheight+top)*SCREENWIDTH-side;
    R_VideoErase (ofs, top*SCREENWIDTH+side);

    // copy sides using wraparound
    ofs = top*SCREENWIDTH + SCREENWIDTH-side;
    side <<= 1;

    for (i=1 ; i<viewheight ; i++)
      {
        R_VideoErase (ofs, side);
        ofs += SCREENWIDTH;
      }
  }
}
