/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: v_video_trans.h,v 1.4 2000/09/16 20:20:43 proff_fs Exp $
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
 *  Video mode translation & image enlarging
 *-----------------------------------------------------------------------------*/

/* CPhipps - Colourmapping
 * Yes, I'm afraid so, it's another colour remapping layer.
 */

#ifndef _V_VIDEO_TRANS_H_
#define _V_VIDEO_TRANS_H_

/* LxDoom may support 24bpp and 32bpp */
typedef unsigned long pval;

typedef struct {
  int pshift; /* Shifts color value to correct place in pixel       */
  int rshift; /* Shift to desired value to make it fit within range */
} colourshift_t;

/* Not really part of this API, but put here as common code 
 * This is the TrueColor palette construction stuff
 */
extern colourshift_t redshift, greenshift, blueshift;
void I_SetColourShift(unsigned long mask, colourshift_t* ps);
void I_SetPaletteTranslation(const byte* palette);

/* These vars must be set before using the functions below */
extern int multiply, dest_bpp;
/* This points to the buffer to output */

/* Macros used in places */
#define BYTESPP ((dest_bpp + 7) / 8)
#define BITSPP  (dest_bpp)

/* Image translation API */

boolean I_QueryImageTranslation(void); /* Is current image translation doable */
size_t I_InitImageTranslation(void); /* Use it then */
extern void (*I_ExpandImage)(pval* dest, const byte* src);
void I_EndImageTranslation(void);

extern boolean expand_buffer; /* Do we need to use expanded buffer */
extern boolean true_color; /* Is this a TrueColor or PseudoColor mode */

extern pval* pixelvals;

#endif
