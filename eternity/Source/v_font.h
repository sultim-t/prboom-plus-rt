// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2004 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// Unified Font Engine
//
// haleyjd 01/14/05
//
//----------------------------------------------------------------------------

#ifndef V_FONT_H__
#define V_FONT_H__

#include "doomtype.h"
#include "r_defs.h"

typedef struct vfont_s
{
   unsigned int start; // first character in font
   unsigned int end;   // last character in font
   unsigned int size;  // number of characters in font

   int cy;    // step amount for \n
   int space; // step for blank space
   int dw;    // width delta (can move characters together)
   int absh;  // absolute maximum height of any character

   boolean color;    // supports color translations?
   boolean upper;    // uses uppercase only?
   boolean centered; // characters are centered in position?

   patch_t **fontgfx; // graphics patches for font

   int cw;  // constant width, used only when centering is on
} vfont_t;

void V_FontWriteText(vfont_t *font, const char *s, int x, int y);
void V_FontWriteTextColored(vfont_t *font, const char *s, int color, int x, int y);
int  V_FontStringHeight(vfont_t *font, const unsigned char *s);
int  V_FontStringWidth(vfont_t *font, const unsigned char *s);

#endif

// EOF

