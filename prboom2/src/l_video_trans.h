/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: l_video_trans.h,v 1.1 2000/05/04 08:08:23 proff_fs Exp $
 *
 *  Video mode translation for LxDoom, 
 *   parts from the original linuxdoom i_video.c
 *  Copyright (C) 1994-1996 by id Software
 *  Copyright (C) 1999 by Colin Phipps
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

#ifndef _L_VIDEO_TRANS_H_
#define _L_VIDEO_TRANS_H_

/* LxDoom may support 24bpp and 32bpp */
typedef unsigned long pval;

typedef struct {
  unsigned char pshift; /* Shifts color value to correct place in pixel       */
  signed   char rshift; /* Shift to desired value to make it fit within range */
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

/*
 * $Log: l_video_trans.h,v $
 * Revision 1.1  2000/05/04 08:08:23  proff_fs
 * Initial revision
 *
 * Revision 1.6  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.5  1999/01/04 19:31:49  cphipps
 * Add RCS log
 * Add extern to I_ExpandImage decl
 *
 */



