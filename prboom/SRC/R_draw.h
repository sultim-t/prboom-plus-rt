// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: R_draw.h,v 1.1 2000/04/09 18:19:05 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __R_DRAW__
#define __R_DRAW__

#include "r_defs.h"

extern lighttable_t *dc_colormap;
extern int      dc_x;
extern int      dc_yl;
extern int      dc_yh;
extern fixed_t  dc_iscale;
extern fixed_t  dc_texturemid;
extern int      dc_texheight;    // killough

// first pixel in a column
extern byte     *dc_source;         

// The span blitting interface.
// Hook in assembler or system specific BLT here.

void R_DrawColumn(void);
void R_DrawTLColumn(void);      // drawing translucent textures // phares
void R_DrawFuzzColumn(void);    // The Spectre/Invisibility effect.

// Draw with color translation tables, for player sprite rendering,
//  Green/Red/Blue/Indigo shirts.

void R_DrawTranslatedColumn(void);

void R_VideoErase(unsigned ofs, int count);

extern lighttable_t *ds_colormap;

extern int     ds_y;
extern int     ds_x1;
extern int     ds_x2;
extern fixed_t ds_xfrac;
extern fixed_t ds_yfrac;
extern fixed_t ds_xstep;
extern fixed_t ds_ystep;

// start of a 64*64 tile image
extern byte *ds_source;              
extern byte *translationtables;
extern byte *dc_translation;

// Span blitting for rows, floor/ceiling. No Spectre effect needed.
void R_DrawSpan(void);

void R_InitBuffer(int width, int height);

// Initialize color translation tables, for player rendering etc.
void R_InitTranslationTables(void);

// Rendering function.
void R_FillBackScreen(void);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder(void);

extern byte *tranmap;         // translucency filter maps 256x256  // phares 
extern byte *main_tranmap;    // killough 4/11/98

#endif

//----------------------------------------------------------------------------
//
// $Log: R_draw.h,v $
// Revision 1.1  2000/04/09 18:19:05  proff_fs
// Initial revision
//
// Revision 1.5  1998/05/03  22:42:23  killough
// beautification, extra declarations
//
// Revision 1.4  1998/04/12  01:58:11  killough
// Add main_tranmap
//
// Revision 1.3  1998/03/02  11:51:55  killough
// Add translucency declarations
//
// Revision 1.2  1998/01/26  19:27:38  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
